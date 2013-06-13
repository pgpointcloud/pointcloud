/***********************************************************************
* pc_point.c
*
*  Pointclound point handling. Create, get and set values from the
*  basic PCPOINT structure.
*
*  PgSQL Pointcloud is free and open source software provided
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include "pc_api_internal.h"
#include "stringbuffer.h"

PCPOINT *
pc_point_make(const PCSCHEMA *s)
{
	size_t sz;
	PCPOINT *pt;

	if ( ! s )
	{
		pcerror("null schema passed into pc_point_make");
		return NULL;
	}

	/* Width of the data area */
	sz = s->size;
	if ( ! sz )
	{
		pcerror("invalid size calculation in pc_point_make");
		return NULL;
	}

	/* Make our own data area */
	pt = pcalloc(sizeof(PCPOINT));
	pt->data = pcalloc(sz);

	/* Set up basic info */
	pt->schema = s;
	pt->readonly = PC_FALSE;
	return pt;
};

PCPOINT *
pc_point_from_data(const PCSCHEMA *s, const uint8_t *data)
{
	size_t sz;
	PCPOINT *pt;
	uint32_t pcid;

	if ( ! s )
	{
		pcerror("null schema passed into pc_point_from_data");
		return NULL;
	}

	/* Reference the external data */
	pt = pcalloc(sizeof(PCPOINT));
	pt->data = (uint8_t*)data;

	/* Set up basic info */
	pt->schema = s;
	pt->readonly = PC_TRUE;
	return pt;
}


void
pc_point_free(PCPOINT *pt)
{
	if ( ! pt->readonly )
	{
		pcfree(pt->data);
	}
	pcfree(pt);
}

int
pc_point_get_double(const PCPOINT *pt, const PCDIMENSION *dim, double *d)
{
	uint8_t *ptr;
	double val;

	/* Read raw value from byte buffer */
	ptr = pt->data + dim->byteoffset;
	val = pc_double_from_ptr(ptr, dim->interpretation);
	val = pc_value_scale_offset(val, dim);

	*d = val;
	return PC_SUCCESS;
}

int
pc_point_get_double_by_name(const PCPOINT *pt, const char *name, double *d)
{
	PCDIMENSION *dim;
	dim = pc_schema_get_dimension_by_name(pt->schema, name);
	if ( ! dim ) return PC_FAILURE;
	return pc_point_get_double(pt, dim, d);
}

int
pc_point_get_double_by_index(const PCPOINT *pt, uint32_t idx, double *d)
{
	PCDIMENSION *dim;
	dim = pc_schema_get_dimension(pt->schema, idx);
	if ( ! dim ) return PC_FAILURE;
	return pc_point_get_double(pt, dim, d);
}

int
pc_point_set_double(PCPOINT *pt, const PCDIMENSION *dim, double val)
{
	uint8_t *ptr;

	/* Remove scale and offsets */
	val = pc_value_unscale_unoffset(val, dim);

	/* Get pointer into byte buffer */
	ptr = pt->data + dim->byteoffset;

	return pc_double_to_ptr(ptr, dim->interpretation, val);
}

int
pc_point_set_double_by_index(PCPOINT *pt, uint32_t idx, double val)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension(pt->schema, idx);
	return pc_point_set_double(pt, d, val);
}

int
pc_point_set_double_by_name(PCPOINT *pt, const char *name, double val)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension_by_name(pt->schema, name);
	return pc_point_set_double(pt, d, val);
}

double
pc_point_get_x(const PCPOINT *pt)
{
	double d;
	pc_point_get_double_by_index(pt, pt->schema->x_position, &d);
	return d;
}

double
pc_point_get_y(const PCPOINT *pt)
{
	double d;
	pc_point_get_double_by_index(pt, pt->schema->y_position, &d);
	return d;
}

double
pc_point_set_x(PCPOINT *pt, double val)
{
	return pc_point_set_double_by_index(pt, pt->schema->x_position, val);
}

double
pc_point_set_y(PCPOINT *pt, double val)
{
	return pc_point_set_double_by_index(pt, pt->schema->y_position, val);
}

char *
pc_point_to_string(const PCPOINT *pt)
{
	/* { "pcid":1, "values":[<dim1>, <dim2>, <dim3>, <dim4>] }*/
	stringbuffer_t *sb = stringbuffer_create();
	char *str;
	int i;

	stringbuffer_aprintf(sb, "{\"pcid\":%d,\"pt\":[", pt->schema->pcid);
	for ( i = 0; i < pt->schema->ndims; i++ )
	{
		double d;
		if ( ! pc_point_get_double_by_index(pt, i, &d) )
		{
			pcerror("pc_point_to_string: unable to read double at position %d", i);
		}
		if ( i )
		{
			stringbuffer_append(sb, ",");
		}
		stringbuffer_aprintf(sb, "%g", d);
	}
	stringbuffer_append(sb, "]}");
	str = stringbuffer_getstringcopy(sb);
	stringbuffer_destroy(sb);
	return str;
}

PCPOINT *
pc_point_from_double_array(const PCSCHEMA *s, double *array, uint32_t nelems)
{
	int i;
	PCPOINT *pt;

	if ( ! s )
	{
		pcerror("null schema passed into pc_point_from_double_array");
		return NULL;
	}

	if ( s->ndims != nelems )
	{
		pcerror("number of elements in schema and array differ in pc_point_from_double_array");
		return NULL;
	}

	/* Reference the external data */
	pt = pcalloc(sizeof(PCPOINT));
	pt->data = pcalloc(s->size);
	pt->schema = s;
	pt->readonly = PC_FALSE;

	for ( i = 0; i < nelems; i++ )
	{
		if ( PC_FAILURE == pc_point_set_double_by_index(pt, i, array[i]) )
		{
			pcerror("failed to write value into dimension %d in pc_point_from_double_array", i);
			return NULL;
		}
	}

	return pt;
}

PCPOINT *
pc_point_from_wkb(const PCSCHEMA *schema, uint8_t *wkb, size_t wkblen)
{
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uchar[]:  data (interpret relative to pcid)
	*/
	const size_t hdrsz = 1+4; /* endian + pcid */
	uint8_t wkb_endian;
	uint32_t pcid;
	uint8_t *data;
	PCPOINT *pt;

	if ( ! wkblen )
	{
		pcerror("pc_point_from_wkb: zero length wkb");
	}

	wkb_endian = wkb[0];
	pcid = wkb_get_pcid(wkb);

	if ( (wkblen-hdrsz) != schema->size )
	{
		pcerror("pc_point_from_wkb: wkb size inconsistent with schema size");
	}

	if ( wkb_endian != machine_endian() )
	{
		/* uncompressed_bytes_flip_endian creates a flipped copy */
		data = uncompressed_bytes_flip_endian(wkb+hdrsz, schema, 1);
	}
	else
	{
		data = pcalloc(schema->size);
		memcpy(data, wkb+hdrsz, wkblen-hdrsz);
	}

	pt = pc_point_from_data(schema, data);
	pt->readonly = PC_FALSE;
	return pt;
}

uint8_t *
pc_point_to_wkb(const PCPOINT *pt, size_t *wkbsize)
{
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uchar[]:  data (interpret relative to pcid)
	*/
	char endian = machine_endian();
	size_t size = 1 + 4 + pt->schema->size;
	uint8_t *wkb = pcalloc(size);
	wkb[0] = endian; /* Write endian flag */
	memcpy(wkb + 1, &(pt->schema->pcid), 4); /* Write PCID */
	memcpy(wkb + 5, pt->data, pt->schema->size); /* Write data */
	if ( wkbsize ) *wkbsize = size;
	return wkb;
}

uint8_t *
pc_point_to_geometry_wkb(const PCPOINT *pt, size_t *wkbsize)
{
	static uint32_t srid_mask = 0x20000000;
	static uint32_t z_mask = 0x80000000;
	uint32_t wkbtype = 1; /* WKB POINT */
	size_t size = 1 + 4 + 8 + 8; /* endian + type + dblX, + dblY */
	uint8_t *wkb, *ptr;
	uint32_t srid;
	int has_srid = PC_FALSE, has_z = PC_FALSE;
	double x, y, z;

	x = pc_point_get_x(pt);
	y = pc_point_get_y(pt);

	if ( pt->schema->srid > 0 )
	{
		has_srid = PC_TRUE;
		wkbtype |= srid_mask;
		size += 4;
		srid = pt->schema->srid;
	}

	if ( pc_point_get_double_by_name(pt, "Z", &z) )
	{
		has_z = PC_TRUE;
		wkbtype |= z_mask;
		size += 8;
	}

	wkb = pcalloc(size);
	ptr = wkb;

	ptr[0] = machine_endian(); /* Endian flag */
	ptr += 1;

	memcpy(ptr, &wkbtype, 4); /* WKB type */
	ptr += 4;

	if ( has_srid )
	{
		memcpy(ptr, &srid, 4); /* SRID */
		ptr += 4;
	}

	memcpy(ptr, &x, 8); /* X */
	ptr += 8;

	memcpy(ptr, &y, 8); /* Y */
	ptr += 8;

	if ( has_z )
	{
		memcpy(ptr, &z, 8); /* Z */
		ptr += 8;
	}

	if ( wkbsize ) *wkbsize = size;
	return wkb;
}
