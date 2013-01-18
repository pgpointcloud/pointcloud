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
	
	sz = c->schema->size;
	
	/* Double the data buffer if it's already full */
	if ( c->npoints == c->maxpoints )
	{
		c->maxpoints *= 2;
		c->data = pcrealloc(c->data, c->maxpoints * sz);
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
pc_patch_make_from_points(const PCPOINT **pts, uint32_t numpts)
{
	PCPATCH *pch;
	const PCSCHEMA *s;
	uint8_t *ptr;
	int i;
	
	if ( ! numpts )
	{
		pcerror("zero point count passed into pc_patch_make_from_points");
		return NULL;
	}

	if ( ! pts )
	{
		pcerror("null point array passed into pc_patch_make_from_points");
		return NULL;
	}
	
	/* Assume the first PCSCHEMA is the same as the rest for now */
	/* We will check this as we go along */
	s = pts[0]->schema;
	
	/* Confirm width of a point data buffer */
	if ( ! s->size )
	{
		pcerror("invalid point size in pc_patch_make_from_points");
		return NULL;
	}

	/* Make our own data area */
	pch = pcalloc(sizeof(PCPATCH)); 
	pch->data = pcalloc(s->size * numpts);
	ptr = pch->data;

	/* Initialize bounds */
	pch->xmin = pch->ymin = MAXFLOAT;
	pch->xmax = pch->ymax = -1 * MAXFLOAT;
	
	/* Set up basic info */
	pch->readonly = PC_FALSE;
	pch->maxpoints = numpts;
	pch->schema = s;
	pch->npoints = 0;

	for ( i = 0; i < numpts; i++ )
	{
		if ( pts[i] )
		{
			if ( pts[i]->schema != s )
			{
				pcerror("points do not share a schema in pc_patch_make_from_points");
				return NULL;
			}
			memcpy(ptr, pts[i]->data, s->size);
			pch->npoints++;
			ptr += s->size;
		}
		else
		{
			pcwarn("encountered null point in pc_patch_make_from_points");
		}
	}
	

	return pch;	
}
