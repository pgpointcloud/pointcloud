/***********************************************************************
* pc_patch_dimensional.c
*
*  Pointclound patch handling. Create, get and set values from the
*  dimensional PCPATCH structure.
*
*  PgSQL Pointcloud is free and open source software provided
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
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
	double xmin, xmax, ymin, ymax, zmin, zmax, mmin, mmax;
	PCSTATS *stats;
	PCBYTES *bytes;
} PCPATCH_DIMENSIONAL;
*/


PCPATCH_DIMENSIONAL *
pc_patch_dimensional_clone(const PCPATCH_DIMENSIONAL *patch)
{
	PCPATCH_DIMENSIONAL *pdl = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
	memcpy(pdl, patch, sizeof(PCPATCH_DIMENSIONAL));
	pdl->bytes = pcalloc(patch->schema->ndims * sizeof(PCBYTES));
	pdl->npoints = 0;
	pdl->stats = NULL;
	return pdl;
}

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
	pc_patch_free((PCPATCH*)patch);
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

	/* Initialize dimensional */
	pdl = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
	pdl->type = PC_DIMENSIONAL;
	pdl->readonly = PC_FALSE;
	pdl->schema = schema;
	pdl->npoints = npoints;
	pdl->bounds = pa->bounds;
	pdl->stats = pc_stats_clone(pa->stats);
	pdl->bytes = pcalloc(ndims * sizeof(PCBYTES));

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
pc_patch_dimensional_compress(const PCPATCH_DIMENSIONAL *pdl, PCDIMSTATS *pds_in)
{
	int i;
	int ndims = pdl->schema->ndims;
	PCPATCH_DIMENSIONAL *pdl_compressed;
	PCDIMSTATS *pds = pds_in;

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
	pdl_compressed->stats = pc_stats_clone(pdl->stats);

	/* Compress each dimension as dictated by stats */
	for ( i = 0; i < ndims; i++ )
	{
		pdl_compressed->bytes[i] = pc_bytes_encode(pdl->bytes[i], pds->stats[i].recommended_compression);
	}

	if ( pds != pds_in ) pc_dimstats_free(pds);

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
	double min, max, avg;
	int rv;
	PCBYTES *pcb;

	assert(pdl);
	assert(pdl->schema);

	pc_bounds_init(&(pdl->bounds), pdl->schema);

	/* Get x extremes */
	if ( pdl->schema->x_position != -1 )
	{
		pcb = &(pdl->bytes[pdl->schema->x_position]);
		rv = pc_bytes_minmax(pcb, &min, &max, &avg);
		if ( PC_FAILURE == rv ) return PC_FAILURE;
		pdl->bounds.xmin = pc_value_scale_offset(min, pdl->schema->dims[pdl->schema->x_position]);
		pdl->bounds.xmax = pc_value_scale_offset(max, pdl->schema->dims[pdl->schema->x_position]);
	}

	/* Get y extremes */
	if ( pdl->schema->y_position != -1 )
	{
		pcb = &(pdl->bytes[pdl->schema->y_position]);
		rv = pc_bytes_minmax(pcb, &min, &max, &avg);
		if ( PC_FAILURE == rv ) return PC_FAILURE;
		pdl->bounds.ymin = pc_value_scale_offset(min, pdl->schema->dims[pdl->schema->y_position]);
		pdl->bounds.ymax = pc_value_scale_offset(max, pdl->schema->dims[pdl->schema->y_position]);
	}

	/* Get z extremes */
	if ( pdl->schema->z_position != -1 )
	{
		pcb = &(pdl->bytes[pdl->schema->z_position]);
		rv = pc_bytes_minmax(pcb, &min, &max, &avg);
		if ( PC_FAILURE == rv ) return PC_FAILURE;
		pdl->bounds.zmin = pc_value_scale_offset(min, pdl->schema->dims[pdl->schema->z_position]);
		pdl->bounds.zmax = pc_value_scale_offset(max, pdl->schema->dims[pdl->schema->z_position]);
	}

	/* Get m extremes */
	if ( pdl->schema->m_position != -1 )
	{
		pcb = &(pdl->bytes[pdl->schema->m_position]);
		rv = pc_bytes_minmax(pcb, &min, &max, &avg);
		if ( PC_FAILURE == rv ) return PC_FAILURE;
		pdl->bounds.mmin = pc_value_scale_offset(min, pdl->schema->dims[pdl->schema->m_position]);
		pdl->bounds.mmax = pc_value_scale_offset(max, pdl->schema->dims[pdl->schema->m_position]);
	}

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
		pcerror("%s: call with wkb that is not dimensionally compressed", __func__);
		return NULL;
	}

	npoints = wkb_get_npoints(wkb);
	ndims = schema->ndims;

	patch = pcalloc(sizeof(PCPATCH_DIMENSIONAL));
	patch->type = PC_DIMENSIONAL;
	patch->readonly = PC_FALSE;
	patch->schema = schema;
	patch->npoints = npoints;
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

	return (PCPATCH*)patch;
}

PCPATCH_DIMENSIONAL *
pc_patch_dimensional_from_pointlist(const PCPOINTLIST *pdl)
{
	PCPATCH_UNCOMPRESSED *patch = pc_patch_uncompressed_from_pointlist(pdl);
	if ( ! patch ) return NULL;
	PCPATCH_DIMENSIONAL *dimpatch = pc_patch_dimensional_from_uncompressed(patch);
	pc_patch_free((PCPATCH*)patch);
	return dimpatch;
}

/** get point n, 0-based, positive */
PCPOINT *pc_patch_dimensional_pointn(const PCPATCH_DIMENSIONAL *pdl, int n)
{
	assert(pdl);
	assert(pdl->schema);
	int i;
	int ndims = pdl->schema->ndims;
	PCPOINT *pt = pc_point_make(pdl->schema);
	uint8_t *buf = pt->data;
	for ( i = 0; i < ndims; i++ )
	{
		PCDIMENSION *dim = pc_schema_get_dimension(pdl->schema, i);
		pc_bytes_to_ptr(buf+dim->byteoffset,pdl->bytes[i], n);
	}

	return pt;
}
