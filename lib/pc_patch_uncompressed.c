/***********************************************************************
* pc_patch_uncompressed.c
*
*  Pointclound patch handling. Create, get and set values from the
*  uncompressed PCPATCH structure.
*
*  PgSQL Pointcloud is free and open source software provided 
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include <float.h>
#include "pc_api_internal.h"
#include "stringbuffer.h"


char *
pc_patch_uncompressed_to_string(const PCPATCH_UNCOMPRESSED *patch)
{
	/* { "pcid":1, "points":[[<dim1>, <dim2>, <dim3>, <dim4>],[<dim1>, <dim2>, <dim3>, <dim4>]] }*/
	stringbuffer_t *sb = stringbuffer_create();
	PCPOINTLIST *pl;
	char *str;
	int i, j;

	pl = pc_pointlist_from_uncompressed(patch);
	stringbuffer_aprintf(sb, "{\"pcid\":%d,\"pts\":[", patch->schema->pcid);
	for ( i = 0; i < pl->npoints; i++ )
	{
		PCPOINT *pt = pc_pointlist_get_point(pl, i);
		if ( i )
		{
			stringbuffer_append(sb, ",");
		}
		stringbuffer_append(sb, "[");
		for ( j = 0; j < pt->schema->ndims; j++ )
		{
			double d;
			if ( ! pc_point_get_double_by_index(pt, j, &d))
			{
				pcerror("pc_patch_to_string: unable to read double at index %d", j);
			}
			if ( j )
			{
				stringbuffer_append(sb, ",");
			}
			stringbuffer_aprintf(sb, "%g", d);
		}
		stringbuffer_append(sb, "]");
	}
	stringbuffer_append(sb, "]}");

	/* All done, copy and clean up */
	pc_pointlist_free(pl);
	str = stringbuffer_getstringcopy(sb);
	stringbuffer_destroy(sb);

	return str;
}

uint8_t *
pc_patch_uncompressed_to_wkb(const PCPATCH_UNCOMPRESSED *patch, size_t *wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:   npoints
    uchar[]:  data (interpret relative to pcid)
	*/
	char endian = machine_endian();
	/* endian + pcid + compression + npoints + datasize */
	size_t size = 1 + 4 + 4 + 4 + patch->datasize;
	uint8_t *wkb = pcalloc(size);
	uint32_t compression = patch->type;
	uint32_t npoints = patch->npoints;
	uint32_t pcid = patch->schema->pcid;
	wkb[0] = endian; /* Write endian flag */
	memcpy(wkb + 1, &pcid,        4); /* Write PCID */
	memcpy(wkb + 5, &compression, 4); /* Write compression */
	memcpy(wkb + 9, &npoints,     4); /* Write npoints */
	memcpy(wkb + 13, patch->data, patch->datasize); /* Write data */
	if ( wkbsize ) *wkbsize = size;
	return wkb;
}


PCPATCH *
pc_patch_uncompressed_from_wkb(const PCSCHEMA *s, const uint8_t *wkb, size_t wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:   npoints
    pcpoint[]:  data (interpret relative to pcid)
	*/
	static size_t hdrsz = 1+4+4+4; /* endian + pcid + compression + npoints */
	PCPATCH_UNCOMPRESSED *patch;
	uint8_t *data;
	uint8_t swap_endian = (wkb[0] != machine_endian());
	uint32_t npoints;

	if ( wkb_get_compression(wkb) != PC_NONE )
	{
		pcerror("pc_patch_uncompressed_from_wkb: call with wkb that is not uncompressed");
		return NULL;
	}

	npoints = wkb_get_npoints(wkb);
	if ( (wkbsize - hdrsz) != (s->size * npoints) )
	{
		pcerror("pc_patch_uncompressed_from_wkb: wkb size and expected data size do not match");
		return NULL;
	}

	if ( swap_endian )
	{
		data = uncompressed_bytes_flip_endian(wkb+hdrsz, s, npoints);
	}
	else
	{
		data = pcalloc(npoints * s->size);
		memcpy(data, wkb+hdrsz, npoints*s->size);
	}

	patch = pcalloc(sizeof(PCPATCH_UNCOMPRESSED));
	patch->npoints = npoints;
    patch->type = PC_NONE;
	patch->maxpoints = npoints;
	patch->schema = s;
	patch->datasize = (wkbsize - hdrsz);
	patch->data = data;
    patch->readonly = PC_FALSE;

	if ( PC_FAILURE == pc_patch_uncompressed_compute_extent(patch) )
		pcerror("pc_patch_uncompressed_compute_extent failed");

	return (PCPATCH*)patch;
}

PCPATCH_UNCOMPRESSED *
pc_patch_uncompressed_make(const PCSCHEMA *s, uint32_t maxpoints)
{
	PCPATCH_UNCOMPRESSED *pch;
	size_t datasize;

	if ( ! s )
	{
		pcerror("null schema passed into pc_patch_uncompressed_make");
		return NULL;
	}

	/* Width of the data area */
	if ( ! s->size )
	{
		pcerror("invalid size calculation in pc_patch_uncompressed_make");
		return NULL;
	}

	/* Make our own data area */
	pch = pcalloc(sizeof(PCPATCH_UNCOMPRESSED));
	datasize = s->size * maxpoints;
	pch->data = pcalloc(datasize);
	pch->datasize = datasize;

	/* Initialize bounds */
	pch->xmin = pch->ymin = FLT_MAX;
	pch->xmax = pch->ymax = -1 * FLT_MAX;

	/* Set up basic info */
	pch->readonly = PC_FALSE;
	pch->npoints = 0;
    pch->stats = NULL;
    pch->type = PC_NONE;
	pch->maxpoints = maxpoints;
	pch->schema = s;
	return pch;
}

int
pc_patch_uncompressed_compute_extent(PCPATCH_UNCOMPRESSED *patch)
{
	int i;
	PCPOINT *pt = pc_point_from_data(patch->schema, patch->data);

	/* Initialize bounds */
	patch->xmin = patch->ymin = FLT_MAX;
	patch->xmax = patch->ymax = -1 * FLT_MAX;

	/* Calculate bounds */
	for ( i = 0; i < patch->npoints; i++ )
	{
		double x, y;
		/* Just push the data buffer forward by one point at a time */
		pt->data = patch->data + i * patch->schema->size;
		x = pc_point_get_x(pt);
		y = pc_point_get_y(pt);
		if ( patch->xmin > x ) patch->xmin = x;
		if ( patch->ymin > y ) patch->ymin = y;
		if ( patch->xmax < x ) patch->xmax = x;
		if ( patch->ymax < y ) patch->ymax = y;
	}

	return PC_SUCCESS;
}

void
pc_patch_uncompressed_free(PCPATCH_UNCOMPRESSED *patch)
{
	if ( ! patch->readonly )
	{
		pcfree(patch->data);
	}
	pcfree(patch);
}



PCPATCH_UNCOMPRESSED *
pc_patch_uncompressed_from_pointlist(const PCPOINTLIST *pl)
{
	PCPATCH_UNCOMPRESSED *pch;
	const PCSCHEMA *s;
    PCPOINT *pt;
	uint8_t *ptr;
	int i;
	uint32_t numpts;

	if ( ! pl )
	{
		pcerror("null PCPOINTLIST passed into pc_patch_uncompressed_from_pointlist");
		return NULL;
	}

	numpts = pl->npoints;
	if ( ! numpts )
	{
		pcerror("zero size PCPOINTLIST passed into pc_patch_uncompressed_from_pointlist");
		return NULL;
	}

	/* Assume the first PCSCHEMA is the same as the rest for now */
	/* We will check this as we go along */
    pt = pc_pointlist_get_point(pl, 0);
	s = pt->schema;

	/* Confirm we have a schema pointer */
	if ( ! s )
	{
		pcerror("pc_patch_uncompressed_from_pointlist: null schema encountered");
		return NULL;
	}

	/* Confirm width of a point data buffer */
	if ( ! s->size )
	{
		pcerror("pc_patch_uncompressed_from_pointlist: invalid point size");
		return NULL;
	}

	/* Make our own data area */
	pch = pcalloc(sizeof(PCPATCH_UNCOMPRESSED));
	pch->datasize = s->size * numpts;
	pch->data = pcalloc(pch->datasize);
	ptr = pch->data;

	/* Initialize bounds */
	pch->xmin = pch->ymin = FLT_MAX;
	pch->xmax = pch->ymax = -1 * FLT_MAX;

	/* Set up basic info */
	pch->readonly = PC_FALSE;
	pch->maxpoints = numpts;
    pch->type = PC_NONE;
	pch->schema = s;
	pch->npoints = 0;

	for ( i = 0; i < numpts; i++ )
	{
        pt = pc_pointlist_get_point(pl, i);
		if ( pt )
		{
			if ( pt->schema->pcid != s->pcid )
			{
				pcerror("pc_patch_uncompressed_from_pointlist: points do not share a schema");
				return NULL;
			}
			memcpy(ptr, pt->data, s->size);
			pch->npoints++;
			ptr += s->size;
		}
		else
		{
			pcwarn("pc_patch_uncompressed_from_pointlist: encountered null point");
		}
	}

	if ( ! pc_patch_compute_extent(pch) )
	{
		pcerror("pc_patch_uncompressed_from_pointlist: failed to compute patch extent");
		return NULL;
	}

	return pch;
}


PCPATCH_UNCOMPRESSED *
pc_patch_uncompressed_from_dimensional(const PCPATCH_DIMENSIONAL *pdl)
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


int
pc_patch_uncompressed_add_point(PCPATCH_UNCOMPRESSED *c, const PCPOINT *p)
{
	size_t sz;
	uint8_t *ptr;
	double x, y;

	if ( ! ( c && p ) )
	{
		pcerror("pc_patch_uncompressed_add_point: null point or patch argument");
		return PC_FAILURE;
	}

	if ( c->schema->pcid != p->schema->pcid )
	{
		pcerror("pc_patch_uncompressed_add_point: pcids of point (%d) and patch (%d) not equal", c->schema->pcid, p->schema->pcid);
		return PC_FAILURE;
	}

	if ( c->readonly )
	{
		pcerror("pc_patch_uncompressed_add_point: cannot add point to readonly patch");
		return PC_FAILURE;
	}

	if ( c->type != PC_NONE )
	{
		pcerror("pc_patch_uncompressed_add_point: cannot add point to compressed patch");
		return PC_FAILURE;
	}

	sz = c->schema->size;

	/* Double the data buffer if it's already full */
	if ( c->npoints == c->maxpoints )
	{
		c->maxpoints *= 2;
		c->datasize = c->maxpoints * sz;
		c->data = pcrealloc(c->data, c->datasize);
	}

	/* Copy the data buffer from point to patch */
	ptr = c->data + sz * c->npoints;
	memcpy(ptr, p->data, sz);
	c->npoints += 1;

	/* Update bounding box */
	x = pc_point_get_x(p);
	y = pc_point_get_y(p);
	if ( c->xmin > x ) c->xmin = x;
	if ( c->ymin > y ) c->ymin = y;
	if ( c->xmax < x ) c->xmax = x;
	if ( c->ymax < y ) c->ymax = y;

	return PC_SUCCESS;
}
