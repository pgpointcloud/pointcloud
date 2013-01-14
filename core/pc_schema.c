/***********************************************************************
* pc_schema.c
*
*  Pointclound schema handling. Parse and emit the XML format for
*  representing packed multidimensional point data.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "pc_api.h"
#include "stringbuffer.h"


/** Convert XML string token to type interpretation number */
static const char * 
pc_interpretation_string(uint32_t interp)
{
	if ( interp >= 0 && interp < NUM_INTERPRETATIONS )
		return INTERPRETATION_STRINGS[interp];
	else
		return "unknown";
}


/** Convert XML string token to type interpretation number */
static int 
pc_interpretation_number(const char *str)
{
	if ( str[0] == 'i' || str[0] == 'I' )
	{
		if ( str[3] == '8' )
			return PC_INT8;
		if ( str[3] == '1' )
			return PC_INT16;
		if ( str[3] == '3' )
			return PC_INT32;
		if ( str[3] == '6' )
			return PC_INT64;
	}
	else if ( str[0] == 'u' || str[0] == 'U' )
	{
		if ( str[4] == '8' )
			return PC_UINT8;
		if ( str[4] == '1' )
			return PC_UINT16;
		if ( str[4] == '3' )
			return PC_UINT32;
		if ( str[4] == '6' )
			return PC_UINT64;
	}
	else if ( str[0] == 'd' || str[0] == 'D' )
	{
		return PC_DOUBLE;
	}
	else if ( str[0] == 'f' || str[0] == 'F' )
	{
		return PC_FLOAT;
	}
	else
		return PC_UNKNOWN;
}

/** Convert type interpretation number size in bytes */
static size_t
pc_interpretation_size(uint32_t interp)
{
	if ( interp >= 0 && interp < NUM_INTERPRETATIONS )
		return INTERPRETATION_SIZES[interp];
	else
		return 0;
}

/** Allocate clean memory for a PCDIMENSION struct */
static PCDIMENSION*
pc_dimension_new()
{
	PCDIMENSION *pcd = pcalloc(sizeof(PCDIMENSION));
	memset(pcd, 0, sizeof(PCDIMENSION));
	return pcd;
}

/** Release the memory behind the PCDIMENSION struct */
static void
pc_dimension_free(PCDIMENSION *pcd)
{
	/* Assumption: No memory in the dimension is owned somewhere else */
	if ( pcd->description )
		pcfree(pcd->description);
	if ( pcd->name )
		pcfree(pcd->name);
	pcfree(pcd);
}

static PCSCHEMA*
pc_schema_new(uint32_t ndims)
{
	PCSCHEMA *pcs = pcalloc(sizeof(PCSCHEMA));
	memset(pcs, 0, sizeof(PCSCHEMA));
	
	pcs->dims = pcalloc(sizeof(PCDIMENSION*) * ndims);
	memset(pcs->dims, 0, sizeof(PCDIMENSION*) * ndims);
	
	pcs->namehash = create_string_hashtable();
	
	pcs->ndims = ndims;
	return pcs;
}

/** Release the memory behind the PCSCHEMA struct */
void 
pc_schema_free(PCSCHEMA *pcs)
{
	int i;

	if ( pcs->namehash )
		hashtable_destroy(pcs->namehash, 0);

	for ( i = 0; i < pcs->ndims; i++ )
	{
		if ( pcs->dims[i] )
		{
			pc_dimension_free(pcs->dims[i]);
			pcs->dims[i] = 0;
		}
	}
	pcfree(pcs->dims);
	pcfree(pcs);
}

/** Convert a PCSCHEMA to a human-readable JSON string */
char * 
pc_schema_to_json(const PCSCHEMA *pcs)
{
	int i;
	char *str;
	stringbuffer_t *sb = stringbuffer_create();
	stringbuffer_append(sb, "{");
	
	if ( pcs->pcid )
		stringbuffer_aprintf(sb, "\"pcid\" : %d,\n", pcs->pcid);
	if ( pcs->srid )
		stringbuffer_aprintf(sb, "\"srid\" : %d,\n", pcs->srid);
	if ( pcs->compression )
		stringbuffer_aprintf(sb, "\"compression\" : %d,\n", pcs->compression);


	if ( pcs->ndims )
	{
		
		stringbuffer_append(sb, "\"dims\" : [\n");
	
		for ( i = 0; i < pcs->ndims; i++ )
		{
			if ( pcs->dims[i] )
			{
				PCDIMENSION *d = pcs->dims[i];

				if ( i ) stringbuffer_append(sb, ",");
				stringbuffer_append(sb, "\n { \n");

				if ( d->name )
					stringbuffer_aprintf(sb, "  \"name\" : \"%s\",\n", d->name);
				if ( d->description )
					stringbuffer_aprintf(sb, "  \"description\" : \"%s\",\n", d->description);

				stringbuffer_aprintf(sb, "  \"size\" : %d,\n", d->size);
				stringbuffer_aprintf(sb, "  \"offset\" : %d,\n", d->offset);
				stringbuffer_aprintf(sb, "  \"scale\" : %g,\n", d->scale);
				stringbuffer_aprintf(sb, "  \"interpretation\" : \"%s\",\n", pc_interpretation_string(d->interpretation));

				stringbuffer_aprintf(sb, "  \"active\" : %d\n", d->active);
				stringbuffer_append(sb, " }");
			}
		}
		stringbuffer_append(sb, "\n]\n");
	}
	stringbuffer_append(sb, "}\n");
	str = stringbuffer_getstringcopy(sb);
	stringbuffer_destroy(sb);
	return str;
}

/** Complete the byte offsets of dimensions from the ordered sizes */
static void
pc_schema_calculate_offsets(const PCSCHEMA *pcs)
{
	int i;
	size_t offset = 0;
	for ( i = 0; i < pcs->ndims; i++ )
	{
		if ( pcs->dims[i] )
		{
			pcs->dims[i]->offset = offset;
			offset += pcs->dims[i]->size;
		}
	}
}

/** Population a PCSCHEMA struct from the XML representation */
PCSCHEMA* pc_schema_construct_from_xml(const char *xml_str)
{
	xmlDocPtr xml_doc = NULL;
	xmlNodePtr xml_root = NULL;
	xmlNsPtr xml_ns = NULL;
	xmlXPathContextPtr xpath_ctx; 
	xmlXPathObjectPtr xpath_obj; 
	xmlNodeSetPtr nodes;
	
	size_t xml_size = strlen(xml_str);
	static xmlChar *xpath_str = "/pc:PointCloudSchema/pc:dimension";
	
	/* Parse XML doc */
	xmlInitParser();
	xml_doc = xmlReadMemory(xml_str, xml_size, NULL, NULL, 0);
	if ( ! xml_doc )
		pcerror("Unable to parse XML");
	
	/* Capture the namespace */
	xml_root = xmlDocGetRootElement(xml_doc);
	if ( xml_root->ns )
		xml_ns = xml_root->ns;
	
	/* Create xpath evaluation context */
	xpath_ctx = xmlXPathNewContext(xml_doc);
	if( ! xpath_ctx )
		pcerror("Unable to create new XPath context");

	/* Register the root namespace if there is one */
	if ( xml_ns )
		xmlXPathRegisterNs(xpath_ctx, "pc", xml_ns->href);
	
	/* Evaluate xpath expression */
	xpath_obj = xmlXPathEvalExpression(xpath_str, xpath_ctx);
	if( ! xpath_obj )
		pcerror("Unable to evaluate xpath expression \"%s\"", xpath_str);

	/* Iterate on the dimensions we found */
	if ( nodes = xpath_obj->nodesetval )
	{
		int ndims = nodes->nodeNr;
		int i;
		PCSCHEMA *s = pc_schema_new(ndims);
				
		for ( i = 0; i < ndims; i++ )
		{
			/* This is a "dimension" */
			if( nodes->nodeTab[i]->type == XML_ELEMENT_NODE )
			{
				xmlNodePtr cur = nodes->nodeTab[i];
				xmlNodePtr child;
				int position = -1;
				PCDIMENSION *d = pc_dimension_new();
				
				/* These are the values of the dimension */
				for ( child = cur->children; child; child = child->next )
				{
					if( child->type == XML_ELEMENT_NODE )
					{
						if ( strcmp(child->name, "name") == 0 )
							d->name = strdup(child->children->content);
						else if ( strcmp(child->name, "description") == 0 )
							d->description = strdup(child->children->content);
						else if ( strcmp(child->name, "size") == 0 )
							d->size = atoi(child->children->content);
						else if ( strcmp(child->name, "active") == 0 )
							d->active = atoi(child->children->content);
						else if ( strcmp(child->name, "position") == 0 )
							d->position = atoi(child->children->content) - 1;
						else if ( strcmp(child->name, "interpretation") == 0 )
							d->interpretation = pc_interpretation_number(child->children->content);
						else if ( strcmp(child->name, "scale") == 0 )
							d->scale = atof(child->children->content);
						else
							pcinfo("Unhandled schema type element \"%s\" encountered", child->name);
					}
				}
				
				/* Store the dimension in the schema */
				if ( d->position >= 0 && d->position < ndims )
				{
					s->dims[d->position] = d;
					d->size = pc_interpretation_size(d->interpretation);
					s->size += d->size;
					hashtable_insert(s->namehash, strdup(d->name), d);
				}
				else
					pcwarn("Dimension at position \"%d\" discarded", position);
			}
		}
		
		/* Complete the byte offsets of dimensions from the ordered sizes */
		pc_schema_calculate_offsets(s);
		return s;
	}
	return NULL;
}

PCDIMENSION *
pc_schema_get_dimension(const PCSCHEMA *s, uint32_t dim)
{
	if ( s && s->ndims > dim && dim >= 0 )
	{
		return s->dims[dim];
	}
	return NULL;
}

size_t
pc_schema_get_size(const PCSCHEMA *s)
{
	return s->size;
}

PCDIMENSION *
pc_schema_get_dimension_by_name(const PCSCHEMA *s, const char *name)
{
	if ( ! ( s && s->namehash ) )
		return NULL;

	return hashtable_search(s->namehash, name);
}
