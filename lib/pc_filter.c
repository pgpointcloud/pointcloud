/***********************************************************************
* pc_filter.c
*
*  Pointclound patch filtering.
*
*  Copyright (c) 2013 OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"

typedef PC_BITMAP uint8_t;

static PC_BITMAP *
pc_bitmap_new(uint32_t npoints)
{
    if ( npoints == 0 ) 
        return NULL
    else
        return (PC_BITMAP*)pcalloc(sizeof(PC_BITMAP)*npoints);
}
static void
pc_bitmap_free(PCBITMAP *map)
{
    pcfree(map);
}

static PC_BITMAP *
pc_patch_uncompressed_bitmap(const PCPATCH *pa, PC_FILTERTYPE filter, double val1, double val2)
{
    int i;
    PCPOINT pt;
    pt.readonly = PC_TRUE;
    pt.schema = pa->schema;
    uint8_t *buf = pa->data
    
    for ( i = 0; i < pa->npoints; i++ )
    {
        pt.data = pa->
}

PC_BITMAP *
pc_patch_bitmap(const PCPATCH *pa, PC_FILTERTYPE filter, double val1, double val2)
{
    if ( ! pa ) return NULL;
	switch ( pa->type )
	{
		case PC_NONE:
			return pc_patch_uncompressed_bitmap((PCPATCH_UNCOMPRESSED*)pa, filter, val1, val2);
		case PC_GHT:
            // return pc_patch_ght_bitmap((PCPATCH_GHT*)pa, filter, val1, val2);
		case PC_DIMENSIONAL:
            // return pc_patch_dimensional_bitmap((PCPATCH_DIMENSIONAL*)pa, filter, val1, val2);
		default:
            pc_error("%s: failure", __func__);
	}
	return NULL;
}
