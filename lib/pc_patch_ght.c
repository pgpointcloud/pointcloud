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

static GhtDimension *
ght_dimension_from_pc_dimension(const PCDIMENSION *pcdim)
{
    int i;
    GhtDimension *dim;
    
    ght_dimension_new(&dim);

    if ( pcdim->name )
    {
        dim->name = pcstrdup(pcdim->name);
    }
    if ( pcdim->description )
    {
        dim->description = pcstrdup(pcdim->description);
    }
    dim->scale = pcdim->scale;
    dim->offset = pcdim->offset;
    dim->type = ght_type_from_pc_type(pcdim->interpretation);
    
    return dim;
}


static GhtSchema *
ght_schema_from_pc_schema(const PCSCHEMA *pcschema)
{
    int i;
    GhtSchema *schema;
    
    ght_schema_new(&schema);

    for ( i = 0; i < pcschema->ndims; i++ )
    {
        GhtDimension *dim = ght_dimension_from_pc_dimension(pcschema->dims[i]);
        ght_schema_add_dimension(schema, dim);
    }

    return schema;
}
#endif /* HAVE_LIBGHT */

void
pc_init_ght_handlers()
{
#ifdef HAVE_LIBGHT

#else
    return;
#endif
}

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
    GhtSchema *schema;
    GhtTree *tree;
    GhtCoordinate coord;
    GhtNode *node;
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
            /* Add attributes to the node */
            for ( j = 0; j < schema->num_dims; j++ )
            {
                PCDIMENSION *dim;
                GhtDimension *ghtdim;
                GhtAttribute *attr;
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
                ght_tree_free(tree);
                return NULL;
            }
        }    
        else
        {
            ght_tree_free(tree);
            return NULL;
        }    
    }
    
    /* Compact the tree */
    if ( ght_tree_compact_attributes(tree) == GHT_OK )
    {
        paght = pcalloc(sizeof(PCPATCH_GHT));
        paght->type = PC_GHT;
        paght->readonly = PC_FALSE;
        paght->schema = pa->schema;
        paght->npoints = pointcount;
        paght->xmin = pa->xmin;
        paght->xmax = pa->xmax;
        paght->ymin = pa->ymin;
        paght->ymax = pa->ymax;
        paght->ght = tree;
    }
    else
    {
        ght_tree_free(tree);
    }
    
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

    if ( paght->ght )
    {
        ght_tree_free(paght->ght);
    }

    pcfree(paght);
#endif
}

#if 0

/* Done */
PCPATCH_UNCOMPRESSED *
pc_patch_uncompressed_from_ght(const PCPATCH_GHT *pdl)
{
    int i, j, npoints;
    PCPATCH_UNCOMPRESSED *patch;
    PCPATCH_DIMENSIONAL *pdl_uncompressed;
    const PCSCHEMA *schema;
    uint8_t *buf;

    npoints = pdl->npoints;
    schema = pdl->schema;
    patch = pcalloc(sizeof(PCPATCH_UNCOMPRESSED));
    patch->schema = schema;
    patch->npoints = npoints;
    patch->maxpoints = npoints;
    patch->readonly = PC_FALSE;
    patch->type = PC_NONE;
    patch->xmin = pdl->xmin;
    patch->xmax = pdl->xmax;
    patch->ymin = pdl->ymin;
    patch->ymax = pdl->ymax;
    patch->datasize = schema->size * pdl->npoints;
    patch->data = pcalloc(patch->datasize);
    buf = patch->data;

    /* Can only read from uncompressed dimensions */
    pdl_uncompressed = pc_patch_dimensional_decompress(pdl);

    for ( i = 0; i < npoints; i++ )
    {
        for ( j = 0; j < schema->ndims; j++ )
        {
            PCDIMENSION *dim = pc_schema_get_dimension(schema, j);
            uint8_t *in = pdl_uncompressed->bytes[j].bytes + dim->size * i;
            uint8_t *out = buf + dim->byteoffset;
            memcpy(out, in, dim->size);
        }
        buf += schema->size;
    }

    pc_patch_dimensional_free(pdl_uncompressed);

    return patch;
}



char *
pc_patch_ght_to_string(const PCPATCH_GHT *pa)
{
    PCPATCH_UNCOMPRESSED *patch = pc_patch_uncompressed_from_ght(pa);
    char *str = pc_patch_uncompressed_to_string(patch);
    pc_patch_uncompressed_free(patch);
    return str;
}


int
pc_patch_ght_compute_extent(PCPATCH_GHT *pdl)
{
	int i;
    double xmin, xmax, ymin, ymax;
    int rv;
    PCBYTES *pcb;

    assert(pdl);
    assert(pdl->schema);

    /* Get x extremes */
    pcb = &(pdl->bytes[pdl->schema->x_position]);
    rv = pc_bytes_minmax(pcb, &xmin, &xmax);
    xmin = pc_value_scale_offset(xmin, pdl->schema->dims[pdl->schema->x_position]);
    xmax = pc_value_scale_offset(xmax, pdl->schema->dims[pdl->schema->x_position]);
    pdl->xmin = xmin;
    pdl->xmax = xmax;

    /* Get y extremes */
    pcb = &(pdl->bytes[pdl->schema->y_position]);
    rv = pc_bytes_minmax(pcb, &ymin, &ymax);
    ymin = pc_value_scale_offset(xmin, pdl->schema->dims[pdl->schema->y_position]);
    ymax = pc_value_scale_offset(xmax, pdl->schema->dims[pdl->schema->y_position]);
    pdl->ymin = ymin;
    pdl->ymax = ymax;

	return PC_SUCCESS;
}

uint8_t *
pc_patch_ght_to_wkb(const PCPATCH_GHT *patch, size_t *wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:   npoints
    dimensions[]:  pcbytes (interpret relative to pcid and compressions)
	*/
    int ndims = patch->schema->ndims;
    int i;
    uint8_t *buf;
	char endian = machine_endian();
	/* endian + pcid + compression + npoints + datasize */
	size_t size = 1 + 4 + 4 + 4 + pc_patch_ght_serialized_size(patch);
	uint8_t *wkb = pcalloc(size);
	uint32_t compression = patch->type;
	uint32_t npoints = patch->npoints;
	uint32_t pcid = patch->schema->pcid;
	wkb[0] = endian; /* Write endian flag */
	memcpy(wkb + 1, &pcid,        4); /* Write PCID */
	memcpy(wkb + 5, &compression, 4); /* Write compression */
	memcpy(wkb + 9, &npoints,     4); /* Write npoints */

    buf = wkb + 13;
    for ( i = 0; i < ndims; i++ )
    {
        size_t bsz;
        PCBYTES *pcb = &(patch->bytes[i]);
// XXX        printf("pcb->(size=%d, interp=%d, npoints=%d, compression=%d, readonly=%d)\n",pcb->size, pcb->interpretation, pcb->npoints, pcb->compression, pcb->readonly);

        pc_bytes_serialize(pcb, buf, &bsz);
        buf += bsz;
    }

	if ( wkbsize ) *wkbsize = size;
	return wkb;
}


PCPATCH *
pc_patch_ght_from_wkb(const PCSCHEMA *schema, const uint8_t *wkb, size_t wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:   npoints
    dimensions[]:  dims (interpret relative to pcid and compressions)
	*/
	static size_t hdrsz = 1+4+4+4; /* endian + pcid + compression + npoints */
	PCPATCH_GHT *patch;
	uint8_t swap_endian = (wkb[0] != machine_endian());
	uint32_t npoints, ndims;
    const uint8_t *buf;
    int i;

	if ( wkb_get_compression(wkb) != PC_DIMENSIONAL )
	{
		pcerror("pc_patch_ght_from_wkb: call with wkb that is not dimensionally compressed");
		return NULL;
	}

	npoints = wkb_get_npoints(wkb);
    ndims = schema->ndims;

	patch = pcalloc(sizeof(PCPATCH_GHT));
	patch->npoints = npoints;
    patch->type = PC_DIMENSIONAL;
	patch->schema = schema;
    patch->readonly = PC_FALSE;
    patch->bytes = pcalloc(ndims*sizeof(PCBYTES));

    buf = wkb+hdrsz;
    for ( i = 0; i < ndims; i++ )
    {
        PCBYTES *pcb = &(patch->bytes[i]);
        PCDIMENSION *dim = schema->dims[i];
        pc_bytes_deserialize(buf, dim, pcb, PC_FALSE /*readonly*/, swap_endian);
        pcb->npoints = npoints;
        buf += pc_bytes_serialized_size(pcb);
    }

	if ( PC_FAILURE == pc_patch_ght_compute_extent(patch) )
		pcerror("pc_patch_ght_compute_extent failed");

	return (PCPATCH*)patch;
}

#endif 

