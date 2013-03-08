/***********************************************************************
* pc_patch_dimensional.c
*
*  Pointclound patch handling. Create, get and set values from the
*  basic PCPATCH structure.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <math.h>
#include <assert.h>
#include "pc_api_internal.h"

/*
typedef struct
{
    int type;
	int8_t readonly;
	const PCSCHEMA *schema;
	uint32_t npoints;
	double xmin, xmax, ymin, ymax;
    PCBYTES *bytes;
} PCPATCH_DIMENSIONAL;
*/


size_t
pc_patch_dimensional_serialized_size(const PCPATCH_DIMENSIONAL *patch)
{
    PCPATCH_DIMENSIONAL *p = (PCPATCH_DIMENSIONAL*)patch;
    int i;
    size_t size = 0;
    for ( i = 0; i < p->schema->ndims; i++ )
    {
        size += pc_bytes_serialized_size(&(p->bytes[i]));
    }
    return size;
}


char *
pc_patch_dimensional_to_string(const PCPATCH_DIMENSIONAL *pa)
{
    PCPATCH_UNCOMPRESSED *patch = pc_patch_uncompressed_from_dimensional(pa);
    char *str = pc_patch_uncompressed_to_string(patch);
    pc_patch_uncompressed_free(patch);
    return str;
}

PCPATCH_DIMENSIONAL *
pc_patch_dimensional_from_uncompressed(const PCPATCH_UNCOMPRESSED *pa)
{
    PCPATCH_DIMENSIONAL *pdl;
    const PCSCHEMA *schema;
    int i, j, ndims, npoints;

    assert(pa);
    npoints = pa->npoints;
    schema = pa->schema;
    ndims = schema->ndims;

    /* Cannot handle empty patches */
    if ( npoints == 0 ) return NULL;

    /* Initialize list */
    pdl = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
    pdl->schema = schema;
    pdl->npoints = npoints;
    pdl->bytes = pcalloc(ndims * sizeof(PCBYTES));
    pdl->readonly = PC_FALSE;
    pdl->type = PC_DIMENSIONAL;
    pdl->xmin = pa->xmin;
    pdl->xmax = pa->xmax;
    pdl->ymin = pa->ymin;
    pdl->ymax = pa->ymax;

    for ( i = 0; i < ndims; i++ )
    {
        PCDIMENSION *dim = pc_schema_get_dimension(schema, i);
        pdl->bytes[i] = pc_bytes_make(dim, npoints);
        for ( j = 0; j < npoints; j++ )
        {
            uint8_t *to = pdl->bytes[i].bytes + dim->size * j;
            uint8_t *from = pa->data + schema->size * j + dim->byteoffset;
            memcpy(to, from, dim->size);
        }
    }
    return pdl;
}

PCPATCH_DIMENSIONAL *
pc_patch_dimensional_compress(const PCPATCH_DIMENSIONAL *pdl, PCDIMSTATS *pds)
{
    int i;
    int ndims = pdl->schema->ndims;
    PCPATCH_DIMENSIONAL *pdl_compressed;

    assert(pdl);
    assert(pdl->schema);

    if ( ! pds )
        pds = pc_dimstats_make(pdl->schema);

    /* Still sampling, update stats */
    if ( pds->total_points < PCDIMSTATS_MIN_SAMPLE )
        pc_dimstats_update(pds, pdl);

    pdl_compressed = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
    memcpy(pdl_compressed, pdl, sizeof(PCPATCH_DIMENSIONAL));
    pdl_compressed->bytes = pcalloc(ndims*sizeof(PCBYTES));

    /* Compress each dimension as dictated by stats */
    for ( i = 0; i < ndims; i++ )
    {
        pdl_compressed->bytes[i] = pc_bytes_encode(pdl->bytes[i], pds->stats[i].recommended_compression);
    }

    return pdl_compressed;
}

PCPATCH_DIMENSIONAL *
pc_patch_dimensional_decompress(const PCPATCH_DIMENSIONAL *pdl)
{
    int i;
    int ndims = pdl->schema->ndims;
    PCPATCH_DIMENSIONAL *pdl_decompressed;

    assert(pdl);
    assert(pdl->schema);

    pdl_decompressed = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
    memcpy(pdl_decompressed, pdl, sizeof(PCPATCH_DIMENSIONAL));
    pdl_decompressed->bytes = pcalloc(ndims*sizeof(PCBYTES));

    /* Compress each dimension as dictated by stats */
    for ( i = 0; i < ndims; i++ )
    {
        pdl_decompressed->bytes[i] = pc_bytes_decode(pdl->bytes[i]);
    }

    return pdl_decompressed;
}

void
pc_patch_dimensional_free(PCPATCH_DIMENSIONAL *pdl)
{
    int i;
    assert(pdl);
    assert(pdl->schema);

    if ( pdl->bytes )
    {
        for ( i = 0; i < pdl->schema->ndims; i++ )
            pc_bytes_free(pdl->bytes[i]);

        pcfree(pdl->bytes);
    }

    pcfree(pdl);
}

int
pc_patch_dimensional_compute_extent(PCPATCH_DIMENSIONAL *pdl)
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
pc_patch_dimensional_to_wkb(const PCPATCH_DIMENSIONAL *patch, size_t *wkbsize)
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
	size_t size = 1 + 4 + 4 + 4 + pc_patch_dimensional_serialized_size(patch);
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
pc_patch_dimensional_from_wkb(const PCSCHEMA *schema, const uint8_t *wkb, size_t wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:   npoints
    dimensions[]:  dims (interpret relative to pcid and compressions)
	*/
	static size_t hdrsz = 1+4+4+4; /* endian + pcid + compression + npoints */
	PCPATCH_DIMENSIONAL *patch;
	uint8_t swap_endian = (wkb[0] != machine_endian());
	uint32_t npoints, ndims;
    const uint8_t *buf;
    int i;

	if ( wkb_get_compression(wkb) != PC_DIMENSIONAL )
	{
		pcerror("pc_patch_dimensional_from_wkb: call with wkb that is not dimensionally compressed");
		return NULL;
	}

	npoints = wkb_get_npoints(wkb);
    ndims = schema->ndims;

	patch = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
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

	if ( PC_FAILURE == pc_patch_dimensional_compute_extent(patch) )
		pcerror("pc_patch_dimensional_compute_extent failed");

	return (PCPATCH*)patch;
}

PCPATCH_DIMENSIONAL *
pc_patch_dimensional_from_pointlist(const PCPOINTLIST *pdl)
{
    PCPATCH_UNCOMPRESSED *patch = pc_patch_uncompressed_from_pointlist(pdl);
    PCPATCH_DIMENSIONAL *dimpatch = pc_patch_dimensional_from_uncompressed(patch);
    pc_patch_uncompressed_free(patch);
    return dimpatch;
}