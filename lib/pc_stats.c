/***********************************************************************
* pc_stats.c
*
*  Pointclound schema handling. Parse and emit the XML format for
*  representing packed multidimensional point data.
*
*  Copyright (c) 2013 OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"

void
pc_stats_free(PCSTATS *pcs)
{
    if ( pcs )
    {
        if ( pcs->stats )
            pcfree(pcs->stats);
        pcfree(pcs);
    }
}

PCSTATS *
pc_stats_new(const PCSCHEMA *schema)
{
    PCSTATS *pcs;
    int i;
    
    /* This situation no sense */
    if ( ! schema || ! schema->ndims ) return NULL;
    
    /* Allocate */
    pcs = pcalloc(sizeof(PCSTATS));
    pcs->stats = pcalloc(schema->ndims * sizeof(PCSTAT));
    pcs->ndims = schema->ndims;
    
    for ( i = 0; i < pcs->ndims; i++ )
    {
        
    }


}
