/***********************************************************************
* pc_filter.c
*
*  Pointclound patch filtering.
*
*  Copyright (c) 2013 OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"

typedef struct
{
    uint32_t nset;
    uint32_t npoints;
    uint8_t *map;
} PCBITMAP;

static PCBITMAP *
pc_bitmap_new(uint32_t npoints)
{
    PCBITMAP *map = pcalloc(sizeof(PCBITMAP));
    map->map = pcalloc(sizeof(uint8_t)*npoints);
    map->npoints = npoints;
    map->nset = 0;
    return map;
}

static void
pc_bitmap_free(PCBITMAP *map)
{
    if (map->map) pcfree(map->map);
    pcfree(map);
}

static inline void
pc_bitmap_set(PCBITMAP *map, int i, int val)
{
    uint8_t curval = map->map[i];
    if ( val && (!curval) )
        map->nset++;
    if ( (!val) && curval )
        map->nset--;
    
    map->map[i] = (val!=0);
}

static inline uint8_t
pc_bitmap_get(const PCBITMAP *map, int i)
{
    return map->map[i];
}

static PCBITMAP *
pc_patch_uncompressed_bitmap(const PCPATCH *pa, const PCDIMENSION *dim, PC_FILTERTYPE filter, double val1, double val2)
{
    PCPOINT pt;
    uint32_t i = 0;    
    uint8_t *buf = pa->data;
    double d;
    size_t sz = pa->schema->size;
    PCBITMAP *map = pc_bitmap_new(pa->npoints);

    pt.readonly = PC_TRUE;
    pt.schema = pa->schema;
    
    while ( i < pa->npoints )
    {
        pt.data = buf;
        pc_point_get_double(&pt, dim, &d);
        switch ( filter )
        {
            case PC_GT:
                pc_bitmap_set(map, i, (d > val1 ? 1 : 0)); 
                break;
            case PC_LT:
                pc_bitmap_set(map, i, (d < val1 ? 1 : 0)); 
                break;
            case PC_EQUAL:
                pc_bitmap_set(map, i, (d == val1 ? 1 : 0)); 
                break;
            case PC_BETWEEN:
                pc_bitmap_set(map, i, (d > val1 && d < val2 ? 1 : 0)); 
                break;
        }
        buf += sz;
        i++;
    }
    
    return map;
}

static PCPATCH_UNCOMPRESSED *
pc_patch_uncompressed_filter(const PCPATCH *pu, const PCBITMAP *map)
{
    int i = 0, j = 0;
    size_t sz = pu->schema->size;
    PCPATCH_UNCOMPRESSED *fpu = pc_patch_uncompressed_new(pu->schema, map->nset);
    uint8_t *buf = pu->data;
    uint8_t *fbuf = fpu->data;
    
    assert(map->npoints == pu->npoints);
    
    while ( i < pu->npoints )
    {
        if ( pc_bitmap_get(map, i) )
        {
            memcpy(fbuf, buf, sz);
            fbuf += sz;
        }
        buf += sz;
        i++;
    }
    
    fpu->maxpoints = fpu->npoints = map->nset;
    
    if ( PC_FAILURE == pc_patch_uncompressed_compute_extent(fpu) )
	{
		pcerror("%s: failed to compute patch extent", __func__);
		return NULL;
	}

	if ( PC_FAILURE == pc_patch_uncompressed_compute_stats(fpu) )
	{
		pcerror("%s: failed to compute patch stats", __func__);
		return NULL;
	}
	
    return fpu;
}

PCPATCH *
pc_patch_filter(const PCPATCH *pa, PC_FILTERTYPE filter, double val1, double val2)
{
    if ( ! pa ) return NULL;
	switch ( pa->type )
	{
		case PC_NONE:
		{
            PCBITMAP *map = pc_patch_uncompressed_bitmap((PCPATCH_UNCOMPRESSED*)pa, filter, val1, val2);
            PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_filter((PCPATCH_UNCOMPRESSED*)pa, map);
            pc_bitmap_free(map);
            return (PCPATCH*)pu;
		}
		case PC_GHT:
            // return pc_patch_ght_bitmap((PCPATCH_GHT*)pa, filter, val1, val2);
		case PC_DIMENSIONAL:
            // return pc_patch_dimensional_bitmap((PCPATCH_DIMENSIONAL*)pa, filter, val1, val2);
		default:
            pc_error("%s: failure", __func__);
	}
	return NULL;
}
