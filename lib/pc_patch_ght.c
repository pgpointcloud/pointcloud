/***********************************************************************
* pc_patch_dght.c
*
*  GHT compressed patch handling. Create, get and set values from the
*  geohashtree (ght) ordered PCPATCH structure.
*
*  PgSQL Pointcloud is free and open source software provided
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include <math.h>
#include <assert.h>
#include "pc_api_internal.h"


/* Includes and functions that expect GHT headers and definitions */

#ifdef HAVE_LIBGHT
static int
pc_type_from_ght_type(const GhtType ghttype)
{
	switch(ghttype)
	{
	case GHT_UNKNOWN:
		return PC_UNKNOWN;
	case GHT_INT8:
		return PC_INT8;
	case GHT_UINT8:
		return PC_UINT8;
	case GHT_INT16:
		return PC_INT16;
	case GHT_UINT16:
		return PC_UINT16;
	case GHT_INT32:
		return PC_INT32;
	case GHT_UINT32:
		return PC_UINT32;
	case GHT_INT64:
		return PC_INT64;
	case GHT_UINT64:
		return PC_UINT64;
	case GHT_DOUBLE:
		return PC_DOUBLE;
	case GHT_FLOAT:
		return PC_FLOAT;
	}
}

static GhtType
ght_type_from_pc_type(const int pctype)
{
	switch(pctype)
	{
	case PC_UNKNOWN:
		return GHT_UNKNOWN;
	case PC_INT8:
		return GHT_INT8;
	case PC_UINT8:
		return GHT_UINT8;
	case PC_INT16:
		return GHT_INT16;
	case PC_UINT16:
		return GHT_UINT16;
	case PC_INT32:
		return GHT_INT32;
	case PC_UINT32:
		return GHT_UINT32;
	case PC_INT64:
		return GHT_INT64;
	case PC_UINT64:
		return GHT_UINT64;
	case PC_DOUBLE:
		return GHT_DOUBLE;
	case PC_FLOAT:
		return GHT_FLOAT;
	}
}

static GhtDimensionPtr
ght_dimension_from_pc_dimension(const PCDIMENSION *d)
{
	int i;
	GhtDimensionPtr dim;
	GhtType type = ght_type_from_pc_type(d->interpretation);
	ght_dimension_new_from_parameters(d->name, d->description, type, d->scale, d->offset, &dim);
	return dim;
}


static GhtSchemaPtr
ght_schema_from_pc_schema(const PCSCHEMA *pcschema)
{
	int i;
	GhtSchemaPtr schema;

	ght_schema_new(&schema);

	for ( i = 0; i < pcschema->ndims; i++ )
	{
		GhtDimensionPtr dim = ght_dimension_from_pc_dimension(pcschema->dims[i]);
		ght_schema_add_dimension(schema, dim);
	}

	return schema;
}

static GhtTreePtr
ght_tree_from_pc_patch(const PCPATCH_GHT *paght)
{
	GhtTreePtr tree;
	GhtReaderPtr reader;
	GhtSchemaPtr ghtschema;

	ghtschema = ght_schema_from_pc_schema(paght->schema);
	if ( ! ghtschema )
		return NULL;

	if ( GHT_OK != ght_reader_new_mem(paght->ght, paght->ghtsize, ghtschema, &reader) )
		return NULL;

	if ( GHT_OK != ght_tree_read(reader, &tree) )
		return NULL;

	return tree;
}
#endif /* HAVE_LIBGHT */

PCPATCH_GHT *
pc_patch_ght_from_pointlist(const PCPOINTLIST *pdl)
{
	PCPATCH_UNCOMPRESSED *patch = pc_patch_uncompressed_from_pointlist(pdl);
	PCPATCH_GHT *ghtpatch = pc_patch_ght_from_uncompressed(patch);
	pc_patch_uncompressed_free(patch);
	return ghtpatch;
}

PCPATCH_GHT *
pc_patch_ght_from_uncompressed(const PCPATCH_UNCOMPRESSED *pa)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return NULL;
#else

	int i, j;
	int pointcount = 0;
	GhtSchemaPtr schema;
	GhtTreePtr tree;
	GhtCoordinate coord;
	GhtNodePtr node;
	GhtErr err;
	PCPOINT pt;
	PCDIMENSION *xdim, *ydim;
	PCPATCH_GHT *paght = NULL;
	size_t pt_size = pa->schema->size;
	double x, y;

	/* Cannot handle empty patches */
	if ( ! pa || ! pa->npoints ) return NULL;

	pt.schema = pa->schema;
	pt.readonly = PC_TRUE;

	xdim = pa->schema->dims[pa->schema->x_position];
	ydim = pa->schema->dims[pa->schema->y_position];

	schema = ght_schema_from_pc_schema(pa->schema);
	if ( ght_tree_new(schema, &tree) != GHT_OK ) return NULL;

	/* Build up the tree from the points. */
	for ( i = 0; i < pa->npoints; i++ )
	{
		pt.data = pa->data + pt_size * i;
		pc_point_get_double(&pt, xdim, &(coord.x));
		pc_point_get_double(&pt, ydim, &(coord.y));

		/* Build a node from the x/y information */
		/* TODO, make resolution configurable from the schema */
		if ( ght_node_new_from_coordinate(&coord, GHT_MAX_HASH_LENGTH, &node) == GHT_OK )
		{
			unsigned int num_dims;
			ght_schema_get_num_dimensions(schema, &num_dims);
			/* Add attributes to the node */
			for ( j = 0; j < num_dims; j++ )
			{
				PCDIMENSION *dim;
				GhtDimensionPtr ghtdim;
				GhtAttributePtr attr;
				double val;

				/* Don't add X or Y as attributes, they are already embodied in the hash */
				if ( j == pa->schema->x_position || j == pa->schema->y_position )
					continue;

				dim = pc_schema_get_dimension(pa->schema, j);
				pc_point_get_double(&pt, dim, &val);

				ght_schema_get_dimension_by_index(schema, j, &ghtdim);
				ght_attribute_new_from_double(ghtdim, val, &attr);
				ght_node_add_attribute(node, attr);
			}

			/* Add the node to the tree */
			/* TODO, make duplicate handling configurable from the schema */
			if ( ght_tree_insert_node(tree, node) == GHT_OK )
			{
				pointcount++;
			}
			else
			{
				// ght_tree_free(tree);
				return NULL;
			}
		}
		else
		{
			// ght_tree_free(tree);
			return NULL;
		}
	}

	/* Compact the tree */
	if ( ght_tree_compact_attributes(tree) == GHT_OK )
	{
		GhtWriterPtr writer;
		paght = pcalloc(sizeof(PCPATCH_GHT));
		paght->type = PC_GHT;
		paght->readonly = PC_FALSE;
		paght->schema = pa->schema;
		paght->npoints = pointcount;
		paght->bounds = pa->bounds;
		paght->stats = pc_stats_clone(pa->stats);

		/* Convert the tree to a memory buffer */
		ght_writer_new_mem(&writer);
		ght_tree_write(tree, writer);
		ght_writer_get_size(writer, &(paght->ghtsize));
		paght->ght = pcalloc(paght->ghtsize);
		ght_writer_get_bytes(writer, paght->ght);
		ght_writer_free(writer);
	}

    // Let the heirarchical memory manager clean up the tree
	// ght_tree_free(tree);
	return paght;
#endif
}

void
pc_patch_ght_free(PCPATCH_GHT *paght)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return;
#else
	int i;
	assert(paght);
	assert(paght->schema);

	/* A readonly tree won't own it's ght buffer, */
	/* so only free a readwrite tree */
	if ( ! paght->readonly )
	{
	    if ( paght->ght )
		    pcfree(paght->ght);
	}

	pcfree(paght);
#endif
}



PCPATCH_UNCOMPRESSED *
pc_patch_uncompressed_from_ght(const PCPATCH_GHT *paght)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return NULL;
#else
	int i, j, npoints;
	PCPATCH_UNCOMPRESSED *patch;
	PCPOINT point;
	const PCSCHEMA *schema;
	GhtNodeListPtr nodelist;
	GhtCoordinate coord;
	GhtNodePtr node;
	GhtTreePtr tree;
	GhtHash *hash;
	GhtAttributePtr attr;

	/* Build a structured tree from the tree serialization */
	if ( ! paght || ! paght->ght ) return NULL;
	tree = ght_tree_from_pc_patch(paght);
	if ( ! tree ) return NULL;

	/* Convert tree to nodelist */
	ght_nodelist_new(paght->npoints, &nodelist);
	ght_tree_to_nodelist(tree, nodelist);

	/* Allocate uncompressed patch */
	ght_nodelist_get_num_nodes(nodelist, &npoints);
	schema = paght->schema;
	patch = pcalloc(sizeof(PCPATCH_UNCOMPRESSED));
	patch->type = PC_NONE;
	patch->readonly = PC_FALSE;
	patch->schema = schema;
	patch->npoints = npoints;
	patch->bounds = paght->bounds;
	patch->stats = pc_stats_clone(paght->stats);
	patch->maxpoints = npoints;
	patch->datasize = schema->size * npoints;
	patch->data = pcalloc(patch->datasize);

	/* Set up utility point */
	point.schema = schema;
	point.readonly = PC_FALSE;
	point.data = patch->data;

	/* Process each point... */
	for ( i = 0; i < npoints; i++ )
	{
		double val;

		/* Read and set X and Y */
		ght_nodelist_get_node(nodelist, i, &node);
		ght_node_get_coordinate(node, &coord);
		pc_point_set_x(&point, coord.x);
		pc_point_set_y(&point, coord.y);

		/* Read and set all the attributes */
		ght_node_get_attributes(node, &attr);
		while ( attr )
		{
			GhtDimensionPtr dim;
			const char *name;
			ght_attribute_get_value(attr, &val);
			ght_attribute_get_dimension(attr, &dim);
			ght_dimension_get_name(dim, &name);
			pc_point_set_double_by_name(&point, name, val);
			ght_attribute_get_next(attr, &attr);
		}
		point.data += schema->size;
	}

	/* Done w/ nodelist and tree */
	ght_nodelist_free_deep(nodelist);
	// ght_tree_free(tree);

	/* Done */
	return patch;
#endif
}


PCPATCH *
pc_patch_ght_from_wkb(const PCSCHEMA *schema, const uint8_t *wkb, size_t wkbsize)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return NULL;
#else
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
	uint32:   npoints
	uint32:   ghtsize
	uint8[]:  ghtbuffer
	*/
	static size_t hdrsz = 1+4+4+4; /* endian + pcid + compression + npoints */
	PCPATCH_GHT *patch;
	uint8_t swap_endian = (wkb[0] != machine_endian());
	uint32_t npoints;
	size_t ghtsize;
	const uint8_t *buf;

	if ( wkb_get_compression(wkb) != PC_GHT )
	{
		pcerror("%s: call with wkb that is not GHT compressed", __func__);
		return NULL;
	}

	npoints = wkb_get_npoints(wkb);

	patch = pcalloc(sizeof(PCPATCH_GHT));
	patch->type = PC_GHT;
	patch->readonly = PC_FALSE;
	patch->schema = schema;
	patch->npoints = npoints;

	/* Start on the GHT */
	buf = wkb+hdrsz;
	ghtsize = wkb_get_int32(buf, swap_endian);
	buf += 4; /* Move to start of GHT buffer */

	/* Copy in the tree buffer */
	patch->ght = pcalloc(ghtsize);
	patch->ghtsize = ghtsize;
	memcpy(patch->ght, buf, ghtsize);

	return (PCPATCH*)patch;
#endif
}


int
pc_patch_ght_compute_extent(PCPATCH_GHT *patch)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return PC_FAILURE;
#else

	GhtTreePtr tree;
	GhtArea area;

	/* Get a tree */
	tree = ght_tree_from_pc_patch(patch);
	if ( ! tree ) return PC_FAILURE;

	/* Calculate bounds and save */
	if ( GHT_OK != ght_tree_get_extent(tree, &area) )
		return PC_FAILURE;

	patch->bounds.xmin = area.x.min;
	patch->bounds.xmax = area.x.max;
	patch->bounds.ymin = area.y.min;
	patch->bounds.ymax = area.y.max;

	// ght_tree_free(tree);

	return PC_SUCCESS;
#endif
}

char *
pc_patch_ght_to_string(const PCPATCH_GHT *pa)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return NULL;
#else
	PCPATCH_UNCOMPRESSED *patch = pc_patch_uncompressed_from_ght(pa);
	char *str = pc_patch_uncompressed_to_string(patch);
	pc_patch_uncompressed_free(patch);
	return str;
#endif
}

uint8_t *
pc_patch_ght_to_wkb(const PCPATCH_GHT *patch, size_t *wkbsize)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return NULL;
#else
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
	uint32:   npoints
	uint32:   ghtsize
	uint8[]:  ghtbuffer
	*/

	uint8_t *buf;
	char endian = machine_endian();
	/* endian + pcid + compression + npoints + ghtsize + ght */
	size_t size = 1 + 4 + 4 + 4 + 4 + patch->ghtsize;

	uint8_t *wkb = pcalloc(size);
	uint32_t compression = patch->type;
	uint32_t npoints = patch->npoints;
	uint32_t pcid = patch->schema->pcid;
	uint32_t ghtsize = patch->ghtsize;
	wkb[0] = endian; /* Write endian flag */
	memcpy(wkb +  1, &pcid,        4); /* Write PCID */
	memcpy(wkb +  5, &compression, 4); /* Write compression */
	memcpy(wkb +  9, &npoints,     4); /* Write npoints */
	memcpy(wkb + 13, &ghtsize,     4); /* Write ght buffer size */

	buf = wkb + 17;
	memcpy(buf, patch->ght, patch->ghtsize);
	if ( wkbsize ) *wkbsize = size;
	return wkb;
#endif
}

PCPATCH_GHT *
pc_patch_ght_filter(const PCPATCH_GHT *patch, uint32_t dimnum, PC_FILTERTYPE filter, double val1, double val2)
{
#ifndef HAVE_LIBGHT
	pcerror("%s: libght support is not enabled", __func__);
	return NULL;
#else
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
	uint32:   npoints
	uint32:   ghtsize
	uint8[]:  ghtbuffer
	*/

	GhtTreePtr tree;
	GhtTreePtr tree_filtered;
    GhtErr err;
	GhtWriterPtr writer;
	GhtArea area;
    const char *dimname;
    const PCDIMENSION *dim;
    PCPATCH_GHT *paght;
    int npoints;

    /* Echo null back */
    if ( ! patch ) return NULL;
    
	/* Get a tree */
	tree = ght_tree_from_pc_patch(patch);
	if ( ! tree ) pcerror("%s: call to ght_tree_from_pc_patch failed", __func__);

    /* Get dimname */
    dim = pc_schema_get_dimension(patch->schema, dimnum);
    if ( ! dim ) pcerror("%s: invalid dimension number (%d)", __func__, dimnum);
    dimname = dim->name;

    switch ( filter )
    {
    	case PC_GT:
            err = ght_tree_filter_greater_than(tree, dimname, val1 > val2 ? val1 : val2, &tree_filtered);
    		break;
    	case PC_LT:
            err = ght_tree_filter_less_than(tree, dimname, val1 < val2 ? val1 : val2, &tree_filtered);
    		break;
    	case PC_EQUAL:
            err = ght_tree_filter_equal(tree, dimname, val1, &tree_filtered);
    		break;
    	case PC_BETWEEN:
            err = ght_tree_filter_between(tree, dimname, val1, val2, &tree_filtered);
    		break;
        default:
            pcerror("%s: invalid filter type (%d)", __func__, filter);
    }

    /* ght_tree_filter_* returns a tree with NULL tree element and npoints == 0 */
    /* for empty filter results (everything got filtered away) */
    if ( err != GHT_OK || ! tree_filtered )
        pcerror("%s: ght_tree_filter failed", __func__);

    /* Read numpoints left in patch */
    ght_tree_get_numpoints(tree_filtered, &(npoints));

    /* Allocate a fresh GHT patch for output */
	paght = pcalloc(sizeof(PCPATCH_GHT));
	paght->type = PC_GHT;
	paght->readonly = PC_FALSE;
	paght->schema = patch->schema;
	paght->npoints = npoints;

	/* No points, not much to do... */	
	if ( ! npoints )
	{
        paght->ghtsize = 0;
        paght->ght = NULL;
    }
    else
    {
    	/* Calculate bounds and save */
    	if ( GHT_OK != ght_tree_get_extent(tree_filtered, &area) )
    		pcerror("%s: ght_tree_get_extent failed", __func__);

    	paght->bounds.xmin = area.x.min;
    	paght->bounds.xmax = area.x.max;
    	paght->bounds.ymin = area.y.min;
    	paght->bounds.ymax = area.y.max;

        /* TODO: Replace this; need to update stats too */
    	paght->stats = pc_stats_clone(patch->stats);    	
    	
    	/* Convert the tree to a memory buffer */
    	ght_writer_new_mem(&writer);
    	ght_tree_write(tree_filtered, writer);
    	ght_writer_get_size(writer, &(paght->ghtsize));
    	paght->ght = pcalloc(paght->ghtsize);
    	ght_writer_get_bytes(writer, paght->ght);
    	ght_writer_free(writer);
	}

	// ght_tree_free(tree_filtered);
	// ght_tree_free(tree);

    return paght;

#endif
}


PCPOINTLIST *
pc_pointlist_from_ght(const PCPATCH_GHT *pag)
{
	PCPATCH_UNCOMPRESSED *pu;
	pu = pc_patch_uncompressed_from_ght(pag);
	return pc_pointlist_from_uncompressed(pu);
}

