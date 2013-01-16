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

PCPATCH * 
pc_patch_make(const PCSCHEMA *s)
{
	PCPATCH *pch;
	uint32_t maxpoints = PCPATCH_DEFAULT_MAXPOINTS;
	
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
	pch->data = pcalloc(s->size * maxpoints);
	
	/* Set up basic info */
	pch->readonly = PC_FALSE;
	pch->npoints = 0;
	pch->maxpoints = maxpoints;
	pch->schema = s;
	pch->xmin = pch->ymin = MAXFLOAT;
	pch->xmax = pch->ymax = -1 * MAXFLOAT;
	return pch;
}

int 
pc_patch_add_point(PCPATCH *c, const PCPOINT *p)
{
	size_t sz;
	uint8_t *ptr;
	
	if ( ! ( c && p ) )
	{
		lwerror("pc_patch_add_point: null point or patch argument");
		return PC_FAILURE;
	}
	
	if ( c->schema->pcid != p->schema->pcid )
	{
		lwerror("pc_patch_add_point: pcids of point (%d) and patch (%d) not equal", c->schema->pcid, p->schema->pcid);
		return PC_FAILURE;
	}
	
	if ( c->readonly )
	{
		lwerror("pc_patch_add_point: cannot add point to readonly patch");
		return PC_FAILURE;
	}
	
	sz = c->schema->size;
	
	/* Double the data buffer if it's already full */
	if ( c->npoints == c->maxpoints )
	{
		c->maxpoints *= 2;
		c->data = pcrealloc(c->data, c->maxpoints * sz);
	}
	
	ptr = c->data + sz * c->npoints;
	memcpy(ptr, p->data, sz);
	c->npoints += 1;
	
	return PC_SUCCESS;	
}

