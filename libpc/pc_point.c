/***********************************************************************
* pc_point.c
*
*  Pointclound point handling. Create, get and set values from the
*  basic PCPOINT structure.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"


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
pc_point_make_from_data(const PCSCHEMA *s, uint8_t *data)
{
	size_t sz;
	PCPOINT *pt;
	
	if ( ! s )
	{
		pcerror("null schema passed into pc_point_make_from_data");
		return NULL;
	}

	/* Reference the external data */
	pt = pcalloc(sizeof(PCPOINT)); 
	pt->data = data;
	
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

static double
pc_point_get_double(const PCPOINT *pt, const PCDIMENSION *d)
{
	uint8_t *ptr;
	double val;
	
	/* Read raw value from byte buffer */
	ptr = pt->data + d->byteoffset;
	val = pc_double_from_ptr(ptr, d->interpretation);

	/* Scale value */
	if ( d->scale )
		val *= d->scale;

	/* Offset value */
	if ( d->offset )
		val += d->offset;
	
	return val;
}

double
pc_point_get_double_by_name(const PCPOINT *pt, const char *name)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension_by_name(pt->schema, name);
	return pc_point_get_double(pt, d);
}

double
pc_point_get_double_by_index(const PCPOINT *pt, uint32_t idx)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension(pt->schema, idx);
	return pc_point_get_double(pt, d);
}

int 
pc_point_set_double(PCPOINT *pt, const PCDIMENSION *d, double val)
{
	uint8_t *ptr;

	/* Offset value */
	if ( d->offset )
		val -= d->offset;

	/* Scale value */
	if ( d->scale )
		val /= d->scale;

	/* Get pointer into byte buffer */
	ptr = pt->data + d->byteoffset;
	
	return pc_double_to_ptr(ptr, d->interpretation, val);
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



