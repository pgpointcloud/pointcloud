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
#include <float.h>

/* PCDOUBLESTAT are members of PCDOUBLESTATS */
typedef struct 
{
    double min;
    double max;
    double sum;
} PCDOUBLESTAT;

/* PCDOUBLESTATS are internal to calculating stats in this module */
typedef struct
{
    PCDOUBLESTAT *dims;
} PCDOUBLESTATS;

/* 
* Instantiate a new PCDOUBLESTATS for calculation, and set up
* initial values for min/max/sum 
*/
static PCDOUBLESTATS *
pc_dstats_new(int ndims)
{
    int i;
    PCDOUBLESTATS *stats = pcalloc(sizeof(PCDOUBLESTATS));
    stats->dims = pcalloc(sizeof(PCDOUBLESTAT)*ndims);
    for ( i = 0; i < ndims; i++ )
    {
        stats->dims[i].min = -1 * DBL_MAX;
        stats->dims[i].max = DBL_MAX;
        stats->dims[i].sum = 0;
    }
    return stats;
}

static void
pc_dstats_free(PCDOUBLESTATS *stats)
{
    if ( ! stats) return;
    if ( stats->dims ) pcfree(stats->dims);
    pcfree(stats);
    return;
}

/**
* Free the standard stats object for in memory patches
*/
static void
pc_stats_free(PCSTATS *stats)
{
    if ( ! stats->min.readonly )
        pcfree(stats->min.data);

    if ( ! stats->max.readonly )
        pcfree(stats->max.data);

    if ( ! stats->avg.readonly )
        pcfree(stats->avg.data);

    pcfree(stats);
    return;
}

/** 
* Build a standard stats object on top of a serialization, allocate just the
* point shells and set the pointers to look into the data area of the 
* serialization.
*/
PCSTATS *
pc_stats_new_from_data(const PCSCHEMA *schema, uint8_t *mindata, uint8_t *maxdata, uint8_t *avgdata)
{
    size_t sz = schema->size;
    PCSTATS *stats = pcalloc(sizeof(PCSTATS));
    /* All share the schema with the patch */
    stats->min.schema = schema;
    stats->max.schema = schema;
    stats->avg.schema = schema;
    /* Data points into serialization */
    stats->min.data = mindata;
    stats->max.data = maxdata;
    stats->avg.data = avgdata;
    /* Can't modify external data */
    stats->min.readonly = PC_TRUE;
    stats->max.readonly = PC_TRUE;
    stats->avg.readonly = PC_TRUE;
    /* Done */
    return stats;
}

/** 
* Build a standard stats object with read/write memory, allocate the
* point shells and the data areas underneath. Used for initial calcution
* of patch stats, when objects first created.
*/
static PCSTATS *
pc_stats_new(const PCSCHEMA *schema)
{
    size_t sz = schema->size;
    PCSTATS *stats = pcalloc(sizeof(PCSTATS));
    stats->min.schema = schema;
    stats->max.schema = schema;
    stats->avg.schema = schema;
    stats->min.readonly = PC_FALSE;
    stats->max.readonly = PC_FALSE;
    stats->avg.readonly = PC_FALSE;
    stats->min.data = pcalloc(schema->size);
    stats->max.data = pcalloc(schema->size);;
    stats->avg.data = pcalloc(schema->size);;
    return stats;
}


static PCSTATS *
pc_stats_new_from_dstats(const PCSCHEMA *schema, const PCDOUBLESTATS *dstats)
{
    int i;
    PCSTATS *stats = pc_stats_new(schema);
    
    for ( i = 0; i < schema->ndims; i++ )
    {
        pc_point_set_double(&(stats->min), schema->dims[i], dstats->dims[i].min);
    }
    return stats;
}


static int 
pc_patch_uncompressed_calculate_stats(PCPATCH_UNCOMPRESSED *pa)
{
    int i, j;
    const PCSCHEMA *schema = pa->schema;
    double val;
    PCDOUBLESTATS *dstats = pc_dstats_new(pa->schema->ndims);
    
    /* Point on stack for fast access to values in patch */
    PCPOINT pt;
    pt.readonly = PC_TRUE;
    pt.schema = schema;
    pt.data = pa->data;
    
    for ( i = 0; i < pa->npoints; i++ )
    {
        for ( j = 0; j < schema->ndims; j++ )
        {
            pc_point_get_double(&pt, schema->dims[j], &val);
            /* Check minimum */
            if ( val < dstats->dims[j].min ) 
                dstats->dims[j].min = val;
            /* Check maximum */
            if ( val > dstats->dims[j].max ) 
                dstats->dims[j].max = val;
            /* Add to sum */
            dstats->dims[j].sum += val;
        }
        /* Advance to next point */
        pt.data += schema->size;
    }
    
    pa->stats = pc_stats_new_from_dstats(pa->schema, dstats);
    pc_dstats_free(dstats);
    return PC_SUCCESS;
}

/**
* Calculate or re-calculate statistics for a patch.
*/
int
pc_patch_calculate_stats(PCPATCH *pa)
{
    if ( ! pa ) return PC_FAILURE;
    if ( pa->stats )
        pcfree(pa->stats);

    switch ( pa->type )
    {
        case PC_DIMENSIONAL:
        {
            pcerror("%s: stats calculation not enabled for patch type %d", __func__, pa->type);
            break;
        }
        case PC_GHT:
        {
            pcerror("%s: stats calculation not enabled for patch type %d", __func__, pa->type);
            break;
        }
        case PC_NONE:
        {
            return pc_patch_uncompressed_calculate_stats((PCPATCH_UNCOMPRESSED*)pa);
        }
        default:
        {
            pcerror("%s: unknown compression type", __func__, pa->type);
            break;
        }
    }

    pcerror("%s: fatal error", __func__);
    return PC_FAILURE;
}



