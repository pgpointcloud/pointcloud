/***********************************************************************
* pc_patch.c
*
*  Pointclound patch handling. Create, get and set values from the
*  basic PCPATCH structure.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <math.h>
#include "pc_api_internal.h"
#include "stringbuffer.h"

PCPATCH * 
pc_patch_make(const PCSCHEMA *s)
{
	PCPATCH *pch;
	uint32_t maxpoints = PCPATCH_DEFAULT_MAXPOINTS;
	size_t datasize;
	
	if ( ! s )
	{
		pcerror("null schema passed into pc_patch_make");
		return NULL;
	}
	
	/* Width of the data area */
	if ( ! s->size )
	{
		pcerror("invalid size calculation in pc_patch_make");
		return NULL;
	}
	
	/* Make our own data area */
	pch = pcalloc(sizeof(PCPATCH)); 
	pch->compressed = PC_FALSE;
	datasize = s->size * maxpoints;
	pch->data = pcalloc(datasize);
	pch->datasize = datasize;

	/* Initialize bounds */
	pch->xmin = pch->ymin = MAXFLOAT;
	pch->xmax = pch->ymax = -1 * MAXFLOAT;
	
	/* Set up basic info */
	pch->readonly = PC_FALSE;
	pch->npoints = 0;
	pch->maxpoints = maxpoints;
	pch->schema = s;
	return pch;
}

static int
pc_patch_compute_extent_uncompressed(PCPATCH *patch)
{
	int i;
	PCPOINT *pt = pc_point_from_data(patch->schema, patch->data);

	/* Initialize bounds */
	patch->xmin = patch->ymin = MAXFLOAT;
	patch->xmax = patch->ymax = -1 * MAXFLOAT;
	
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

static int
pc_patch_compute_extent(PCPATCH *patch)
{
	switch( patch->schema->compression )
	{
		case PC_NONE:
			return pc_patch_compute_extent_uncompressed(patch);
		case PC_GHT:
			return PC_FAILURE;
		case PC_DIMENSIONAL:
			return PC_FAILURE;
	}
	return PC_FAILURE;
}

void
pc_patch_free(PCPATCH *patch)
{
	if ( ! patch->readonly )
	{
		pcfree(patch->data);
	}
	pcfree(patch);
}

int 
pc_patch_add_point(PCPATCH *c, const PCPOINT *p)
{
	size_t sz;
	uint8_t *ptr;
	double x, y;
	
	if ( ! ( c && p ) )
	{
		pcerror("pc_patch_add_point: null point or patch argument");
		return PC_FAILURE;
	}
	
	if ( c->schema->pcid != p->schema->pcid )
	{
		pcerror("pc_patch_add_point: pcids of point (%d) and patch (%d) not equal", c->schema->pcid, p->schema->pcid);
		return PC_FAILURE;
	}

	if ( c->readonly )
	{
		pcerror("pc_patch_add_point: cannot add point to readonly patch");
		return PC_FAILURE;
	}

	if ( c->compressed && c->schema->compression != PC_NONE )
	{
		pcerror("pc_patch_add_point: cannot add point to compressed patch");
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



PCPATCH * 
pc_patch_from_points(const PCPOINTLIST *pl)
{
	PCPATCH *pch;
	const PCSCHEMA *s;
	uint8_t *ptr;
	int i;
	uint32_t numpts;
	
	if ( ! pl )
	{
		pcerror("null PCPOINTLIST passed into pc_patch_from_points");
		return NULL;
	}

	numpts = pl->npoints;
	if ( ! numpts )
	{
		pcerror("zero size PCPOINTLIST passed into pc_patch_from_points");
		return NULL;
	}

	/* Assume the first PCSCHEMA is the same as the rest for now */
	/* We will check this as we go along */
	s = pl->points[0]->schema;

	/* Confirm we have a schema pointer */
	if ( ! s )
	{
		pcerror("pc_patch_from_points: null schema encountered");
		return NULL;
	}
	
	/* Confirm width of a point data buffer */
	if ( ! s->size )
	{
		pcerror("pc_patch_from_points: invalid point size");
		return NULL;
	}

	/* Make our own data area */
	pch = pcalloc(sizeof(PCPATCH)); 
	pch->datasize = s->size * numpts;
	pch->data = pcalloc(pch->datasize);
	ptr = pch->data;

	/* Initialize bounds */
	pch->xmin = pch->ymin = MAXFLOAT;
	pch->xmax = pch->ymax = -1 * MAXFLOAT;
	
	/* Set up basic info */
	pch->readonly = PC_FALSE;
	pch->compressed = PC_FALSE;
	pch->maxpoints = numpts;
	pch->schema = s;
	pch->npoints = 0;

	for ( i = 0; i < numpts; i++ )
	{
		if ( pl->points[i] )
		{
			if ( pl->points[i]->schema->pcid != s->pcid )
			{
				pcerror("pc_patch_from_points: points do not share a schema");
				return NULL;
			}
			memcpy(ptr, pl->points[i]->data, s->size);
			pch->npoints++;
			ptr += s->size;
		}
		else
		{
			pcwarn("pc_patch_from_points: encountered null point");
		}
	}

	if ( ! pc_patch_compute_extent(pch) )
	{
		pcerror("pc_patch_from_points: failed to compute patch extent");
		return NULL;
	}

	return pch;	
}


PCPOINTLIST *
pc_patch_to_points_uncompressed(const PCPATCH *patch)
{
	int i;
	PCPOINTLIST *pl;
	size_t pt_size = patch->schema->size;
	uint32_t npoints = patch->npoints;
	
	pl = pc_pointlist_make(npoints);
	for ( i = 0; i < npoints; i++ )
	{
		pc_pointlist_add_point(pl, pc_point_from_data(patch->schema, patch->data + i*pt_size));
	}
	return pl;
}

PCPOINTLIST * 
pc_patch_to_points(const PCPATCH *patch)
{
	uint32_t compression = patch->schema->compression;

	if ( ! patch->compressed )
		return pc_patch_to_points_uncompressed(patch);
		
	switch ( compression )
	{
		case PC_NONE:
		{
			return pc_patch_to_points_uncompressed(patch);
		}
		case PC_GHT:
		{
			// return pc_patch_to_points_ght(patch);
		}
		case PC_DIMENSIONAL:
		{
			// return pc_patch_to_points_dimensional(patch);
		}
	}
	
	/* Don't get here */
	pcerror("pc_patch_to_points: unsupported compression type %d", compression);
	return NULL;
}


static PCPATCH *
pc_patch_compress_dimensional(const PCPATCH *patch)
{
	pcerror("pc_patch_compress_dimensional unimplemented");
	return NULL;
}

static PCPATCH *
pc_patch_compress_ght(const PCPATCH *patch)
{
	pcerror("pc_patch_compress_ght unimplemented");
	return NULL;
}
	

PCPATCH *
pc_patch_compress(const PCPATCH *patch)
{
	uint32_t compression = patch->schema->compression;

	if ( patch->compressed )
		return pc_patch_clone(patch);
		
	switch ( compression )
	{
		case PC_NONE:
		{
			PCPATCH *newpatch = pc_patch_clone(patch);
			newpatch->compressed = PC_TRUE;
			return newpatch;
		}
		case PC_GHT:
		{
			return pc_patch_compress_ght(patch);
		}
		case PC_DIMENSIONAL:
		{
			return pc_patch_compress_dimensional(patch);
		}
	}
	
	/* Don't get here */
	pcerror("pc_patch_compress: unknown compression type %d", compression);
	return NULL;
}

PCPATCH *
pc_patch_clone(const PCPATCH *patch)
{
	PCPATCH *newpatch = pcalloc(sizeof(PCPATCH));
	memcpy(newpatch, patch, sizeof(PCPATCH));
	newpatch->data = pcalloc(newpatch->datasize);
	memcpy(newpatch->data, patch->data, newpatch->datasize);
	return newpatch;
}


static PCPATCH * 
pc_patch_from_wkb_uncompressed(const PCSCHEMA *s, uint8_t *wkb, size_t wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:   npoints
    pcpoint[]:  data (interpret relative to pcid)
	*/
	static size_t hdrsz = 1+4+4+4; /* endian + pcid + compression + npoints */
	PCPATCH *patch;
	uint8_t *data;
	uint8_t swap_endian = (wkb[0] != machine_endian());
	uint32_t npoints;
	
	if ( wkb_get_compression(wkb) != PC_NONE )
	{
		pcerror("pc_patch_from_wkb_uncompressed: call with wkb that is not uncompressed");
		return NULL;
	}

	npoints = wkb_get_npoints(wkb);
	if ( (wkbsize - hdrsz) != (s->size * npoints) )
	{
		pcerror("pc_patch_from_wkb_uncompressed: wkb size and expected data size do not match");
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
	
	patch = pcalloc(sizeof(PCPATCH));
	patch->npoints = npoints;
	patch->maxpoints = npoints;
	patch->schema = s;
	patch->compressed = PC_TRUE; /* It's in whatever compression it came in */
	patch->datasize = (wkbsize - hdrsz);
	patch->data = data;

	if ( PC_FAILURE == pc_patch_compute_extent(patch) )
	{
		pcerror("pc_patch_compute_extent failed");
	}

	return patch;
}



PCPATCH * 
pc_patch_from_wkb(const PCSCHEMA *s, uint8_t *wkb, size_t wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uchar[]:  data (interpret relative to pcid and compression)
	*/
	uint32_t compression, pcid;
	
	if ( ! wkbsize )
	{
		pcerror("pc_patch_from_wkb: zero length wkb");
	}
	
	pcid = wkb_get_pcid(wkb);
	compression = wkb_get_compression(wkb);

	// if ( compression != s->compression )
	// {
	// 	pcerror("pc_patch_from_wkb: wkb compression (%d) not consistent with schema compression (%d)", compression, s->compression);
	// }
	if ( pcid != s->pcid )
	{
		pcerror("pc_patch_from_wkb: wkb pcid (%d) not consistent with schema pcid (%d)", pcid, s->pcid);
	}
	
	switch ( compression )
	{
		case PC_NONE:
		{
			return pc_patch_from_wkb_uncompressed(s, wkb, wkbsize);
		}
		case PC_GHT:
		{
			pcerror("pc_patch_from_wkb: GHT compression not yet supported");
			return NULL;
		}
		case PC_DIMENSIONAL:
		{
			pcerror("pc_patch_from_wkb: Dimensional compression not yet supported");
			return NULL;
		}
	}
	
	/* Don't get here */
	pcerror("pc_patch_from_wkb: unknown compression '%d' requested", compression);
	return NULL;
}

static uint8_t *
pc_patch_to_wkb_uncompressed(const PCPATCH *patch, size_t *wkbsize)
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
	uint32_t compression = patch->schema->compression;
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

uint8_t *
pc_patch_to_wkb(const PCPATCH *patch, size_t *wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uchar[]:  data (interpret relative to pcid and compression)
	*/

	switch ( patch->schema->compression )
	{
		case PC_NONE:
		{
			return pc_patch_to_wkb_uncompressed(patch, wkbsize);
		}
		case PC_GHT:
		{
			pcerror("pc_patch_to_wkb: GHT compression not yet supported");
			return NULL;
		}
		case PC_DIMENSIONAL:
		{
			pcerror("pc_patch_to_wkb: Dimensional compression not yet supported");
			return NULL;
		}
	}	
	pcerror("pc_patch_to_wkb: unknown compression requested '%d'", patch->schema->compression);
	return NULL;
}

char *
pc_patch_to_string(const PCPATCH *patch)
{
	/* ( <pcid> : (<dim1>, <dim2>, <dim3>, <dim4>), (<dim1>, <dim2>, <dim3>, <dim4>))*/
	stringbuffer_t *sb = stringbuffer_create();
	PCPOINTLIST *pl;
	char *str;
	int i, j;
	
	pl = pc_patch_to_points(patch);
	stringbuffer_aprintf(sb, "[ %d : ", patch->schema->pcid);
	for ( i = 0; i < pl->npoints; i++ )
	{
		PCPOINT *pt = pl->points[i];
		if ( i )
		{
			stringbuffer_append(sb, ", ");
		}
		stringbuffer_append(sb, "(");
		for ( j = 0; j < pt->schema->ndims; j++ )
		{
			double d;
			if ( ! pc_point_get_double_by_index(pt, j, &d))
			{
				pcerror("pc_patch_to_string: unable to read double at index %d", j);
			}
			if ( j )
			{
				stringbuffer_append(sb, ", ");
			}
			stringbuffer_aprintf(sb, "%g", d);
		}
		stringbuffer_append(sb, ")");
	}
	stringbuffer_append(sb, " ]");

	/* All done, copy and clean up */
	pc_pointlist_free(pl);
	str = stringbuffer_getstringcopy(sb);
	stringbuffer_destroy(sb);

	return str;
}

static uint8_t *
pc_patch_wkb_set_double(uint8_t *wkb, double d)
{
	memcpy(wkb, &d, 8);
	wkb += 8;
	return wkb;
}

static uint8_t *
pc_patch_wkb_set_int32(uint8_t *wkb, uint32_t i)
{
	memcpy(wkb, &i, 8);
	wkb += 4;
	return wkb;
}

static uint8_t *
pc_patch_wkb_set_char(uint8_t *wkb, char c)
{
	memcpy(wkb, &c, 1);
	wkb += 1;
	return wkb;
}

uint8_t *
pc_patch_to_geometry_wkb_envelope(const PCPATCH *pa, size_t *wkbsize)
{
	static uint32_t srid_mask = 0x20000000;
	static uint32_t nrings = 1;
	static uint32_t npoints = 5;
	uint32_t wkbtype = 3; /* WKB POLYGON */
	uint8_t *wkb, *ptr;
	int has_srid = PC_FALSE;
	size_t size = 1 + 4 + 4 + 4 + 2*npoints*8; /* endian + type + nrings + npoints + 5 dbl pts */
	double x, y;
	
	if ( pa->schema->srid > 0 )
	{
		has_srid = PC_TRUE;
		wkbtype |= srid_mask;
		size += 4;
	}

	wkb = pcalloc(size);
	ptr = wkb;
	
	ptr = pc_patch_wkb_set_char(ptr, machine_endian()); /* Endian flag */
	
	ptr = pc_patch_wkb_set_int32(ptr, wkbtype); /* TYPE = Polygon */
	
	if ( has_srid )
	{
		ptr = pc_patch_wkb_set_int32(ptr, pa->schema->srid); /* SRID */
	}
	
	ptr = pc_patch_wkb_set_int32(ptr, nrings);  /* NRINGS = 1 */
	ptr = pc_patch_wkb_set_int32(ptr, npoints); /* NPOINTS = 5 */
	
	/* Point 0 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmin);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymin);
	
	/* Point 1 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmin);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymax);
	
	/* Point 2 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmax);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymax);
	
	/* Point 3 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmax);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymin);
	
	/* Point 4 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmin);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymin);
	
	if ( wkbsize ) *wkbsize = size;
	return wkb;
}