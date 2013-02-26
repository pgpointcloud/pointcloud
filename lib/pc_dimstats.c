/***********************************************************************
* pc_dimstats.c
*
*  Support for "dimensional compression", which is a catch-all
*  term for applying compression separately on each dimension
*  of a PCPATCH collection of PCPOINTS.
*
*  Depending on the character of the data, one of these schemes
*  will be used:
*
*  - run-length encoding
*  - significant-bit removal
*  - deflate
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <stdarg.h>
#include <assert.h>
#include "pc_api_internal.h"


PCDIMSTATS *
pc_dimstats_make(const PCSCHEMA *schema)
{
    PCDIMSTATS *pds;
    pds = pcalloc(sizeof(PCDIMSTATS));
    pds->ndims = schema->ndims;
    pds->stats = pcalloc(pds->ndims * sizeof(PCDIMSTAT));
    return pds;
}

void
pc_dimstats_free(PCDIMSTATS *pds)
{
    if ( pds->stats )
        pcfree(pds->stats);
    pcfree(pds);
}

int
pc_dimstats_update(PCDIMSTATS *pds, const PCDIMLIST *pdl)
{
    int i, j;
    uint32_t nelems = pdl->npoints;
    const PCSCHEMA *schema = pdl->schema;
    
    /* Update global stats */
    pds->total_points += pdl->npoints;
    pds->total_patches += 1;
    
    /* Update dimensional stats */
    for ( i = 0; i < pds->ndims; i++ )
    {
        PCBYTES pcb = pdl->bytes[i];
        pds->stats[i].total_runs += pc_bytes_run_count(&pcb);
        pds->stats[i].total_commonbits += pc_bytes_sigbits_count(&pcb);
    }
    
    /* Update recommended compression schema */
    for ( i = 0; i < pds->ndims; i++ )
    {
        PCDIMENSION *dim = pc_schema_get_dimension(schema, i);
        /* Uncompressed size, foreach point, one value entry */
        double raw_size = pds->total_points * dim->size;
        /* RLE size, for each run, one count byte and one value entry */
        double rle_size = pds->stats[i].total_runs * (dim->size + 1);
        /* Sigbits size, for each patch, one header and n bits for each entry */
        double avg_commonbits_per_patch = pds->stats[i].total_commonbits / pds->total_patches;
        double avg_uniquebits_per_patch = 8*dim->size - avg_commonbits_per_patch;
        double sigbits_size = pds->total_patches * 2 * dim->size + pds->total_points * avg_uniquebits_per_patch / 8;
        /* Default to ZLib */
        pds->stats[i].recommended_compression = PC_DIM_ZLIB;
        /* Only use rle and sigbits compression on integer values */
        /* If we can do better than 4:1 we might beat zlib */
        if ( dim->interpretation != PC_DOUBLE )
        {
            /* If sigbits is better than 4:1, use that */
            if ( raw_size/sigbits_size > 4.0 )
            {
                pds->stats[i].recommended_compression = PC_DIM_SIGBITS;
            }
            /* If RLE size is even better, use that. */
            else if ( raw_size/rle_size > 4.0 )
            {
                pds->stats[i].recommended_compression = PC_DIM_RLE;
            }
        }
    }
    return PC_SUCCESS;
}
