/***********************************************************************
* pc_pointlist.c
*
*  Point list handling. Create, get and set values from the
*  basic PCPOINTLIST structure.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"
#include <assert.h>

PCPOINTLIST *
pc_pointlist_make(uint32_t npoints)
{
	PCPOINTLIST *pl = pcalloc(sizeof(PCPOINTLIST));
	pl->points = pcalloc(sizeof(PCPOINT*) * npoints);
	pl->maxpoints = npoints;
	pl->npoints = 0;
	pl->readonly = PC_FALSE;
	return pl;
}

void
pc_pointlist_free(PCPOINTLIST *pl)
{
	int i;
	for ( i = 0; i < pl->npoints; i++ )
	{
		pc_point_free(pl->points[i]);
	}
	pcfree(pl->points);
	pcfree(pl);
	return;
}

void
pc_pointlist_add_point(PCPOINTLIST *pl, PCPOINT *pt)
{
	if ( pl->npoints >= pl->maxpoints )
	{
		if ( pl->maxpoints < 1 ) pl->maxpoints = 1;
		pl->maxpoints *= 2;
		pl->points = pcrealloc(pl->points, pl->maxpoints * sizeof(PCPOINT*));
	}
	
	pl->points[pl->npoints] = pt;
	pl->npoints += 1;
	return;
}


PCPOINTLIST *
pc_pointlist_from_dimlist(PCDIMLIST *pdl)
{
    PCPOINTLIST *pl;
    int i, j, ndims, npoints;
    assert(pdl);
    
    /* We can only pull off this trick on uncompressed data */
    if ( PC_FAILURE == pc_dimlist_decode(pdl) )
        return NULL;
    
    ndims = pdl->schema->ndims;
    npoints = pdl->npoints;
    pl = pc_pointlist_make(npoints);
    
    for ( i = 0; i < npoints; i++ )
    {
        PCPOINT *pt = pc_point_make(pdl->schema);
        for ( j = 0; j < ndims; j++ )
        {
            PCDIMENSION *dim = pc_schema_get_dimension(pdl->schema, j);
            
            uint8_t *in = pdl->bytes[j].bytes + dim->size * i;
            uint8_t *out = pt->data + dim->byteoffset;
            memcpy(out, in, dim->size);
        }
        pc_pointlist_add_point(pl, pt);
    }
    return pl;
}
