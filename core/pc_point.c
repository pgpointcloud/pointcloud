/***********************************************************************
* pc_point.c
*
*  Pointclound point handling. Create, get and set values from the
*  basic PCPOINT structure.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_api.h"

PCPOINT* pc_point_new(const PCSCHEMA *s, uint8_t *data)
{
	int i;
	size_t sz = 0;
	PCPOINT *pt;
	
	if ( ! s )
	{
		pcerror("null schema passed into pc_point_new");
		return NULL;
	}
	
	/* Calculate total data width of the PCPOINT */
	for ( i = 0; i < s->ndims; i++ )
	{
		if ( s->ndims[i] )
			sz += s->dims[i]->size;
	}
	
	/* PCPOINT struct already has 1 byte alloc'ed for data */
	pt = pcalloc(sizeof(PCPOINT) + (sz - 1)); 
	
	/* Set up basic info */
	pt->pcid = s->pcid;

	
};

