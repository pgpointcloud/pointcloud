/***********************************************************************
* pc_dimlist.c
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

#include <assert.h>
#include "pc_api_internal.h"



/**
* Converts a list of I N-dimensional points into a 
* list of N I-valued dimensions. Precursor to running
* compression on each dimension separately.
*/
PCDIMLIST *
pc_dimlist_from_pointlist(const PCPOINTLIST *pl)
{
    PCDIMLIST *pdl;
    int i, j, ndims, npoints;
    assert(pl);
    
    if ( pl->npoints == 0 ) return NULL;
    
    pdl = pcalloc(sizeof(PCDIMLIST));
    pdl->schema = pl->points[0]->schema;
    ndims = pdl->schema->ndims;
    npoints = pl->npoints;
    pdl->npoints = npoints;
    pdl->bytes = pcalloc(ndims * sizeof(PCBYTES));
    
    for ( i = 0; i < ndims; i++ )
    {
        PCDIMENSION *dim = pc_schema_get_dimension(pdl->schema, i);
        pdl->bytes[i] = pc_bytes_make(dim, npoints);
        for ( j = 0; j < npoints; j++ )
        {
            uint8_t *to = pdl->bytes[i].bytes + dim->size * j;
            uint8_t *from = pl->points[j]->data + dim->byteoffset;
            memcpy(to, from, dim->size);
        }
    }    
    return pdl;
}

void
pc_dimlist_free(PCDIMLIST *pdl)
{
    int i;
    assert(pdl);
    assert(pdl->schema);
    
    if ( pdl->bytes )
    {
        for ( i = 0; i < pdl->schema->ndims; i++ )
            pc_bytes_free(pdl->bytes[i]);

        pcfree(pdl->bytes);
    }
    
    pcfree(pdl);
}

int
pc_dimlist_encode(PCDIMLIST *pdl, PCDIMSTATS **pdsptr)
{
    int i;
    PCDIMSTATS *pds;
    
    assert(pdl);
    assert(pdl->schema);
    assert(pdsptr);
    
    /* Maybe we have stats passed in */
    pds = *pdsptr;
    
    /* No stats at all, make a new one */
    if ( ! pds )
        pds = pc_dimstats_make(pdl->schema);

    /* Still sampling, update stats */
    if ( pds->total_points < PCDIMSTATS_MIN_SAMPLE )
        pc_dimstats_update(pds, pdl);
    
    /* Compress each dimension as dictated by stats */
    for ( i = 0; i < pdl->schema->ndims; i++ )
    {
        pdl->bytes[i] = pc_bytes_encode(pdl->bytes[i], pds->stats[i].recommended_compression);
    }
    
    return PC_SUCCESS;
}

int
pc_dimlist_decode(PCDIMLIST *pdl)
{
    int i;
    int ndims;
    assert(pdl);
    assert(pdl->schema);

    /* Compress each dimension as dictated by stats */
    for ( i = 0; i < pdl->schema->ndims; i++ )
    {
        pdl->bytes[i] = pc_bytes_decode(pdl->bytes[i]);
    }    

    return PC_SUCCESS;
}

PCDIMLIST *
pc_dimlist_from_patch(const PCPATCH *pa)
{
    PCDIMLIST *pdl;
    const PCSCHEMA *schema;
    int i, j, ndims, npoints;

    assert(pa);
    npoints = pa->npoints;
    schema = pa->schema;
    ndims = schema->ndims;

    /* Uncompressed patches only! */
    if ( pa->compressed )
		pcerror("pc_dimlist_from_patch: cannot operate on compressed patch");
    
    /* Cannot handle empty patches */
    if ( npoints == 0 ) return NULL;
    
    /* Initialize list */
    pdl = pcalloc(sizeof(PCDIMLIST));
    pdl->schema = schema;
    pdl->npoints = npoints;
    pdl->bytes = pcalloc(ndims * sizeof(PCBYTES));
    
    for ( i = 0; i < ndims; i++ )
    {
        PCDIMENSION *dim = pc_schema_get_dimension(schema, i);
        pdl->bytes[i] = pc_bytes_make(dim, npoints);
        for ( j = 0; j < npoints; j++ )
        {
            uint8_t *to = pdl->bytes[i].bytes + dim->size * j;
            uint8_t *from = pa->data + schema->size * j + dim->byteoffset;
            memcpy(to, from, dim->size);
        }
    }
    return pdl;    
}


uint8_t *
pc_dimlist_serialize(const PCDIMLIST *pdl, size_t *size)
{
    int i;
    int ndims = pdl->schema->ndims;
    size_t dlsize = 0;
    uint8_t *buf;
    
    for ( i = 0; i < ndims; i++ )
        dlsize += pc_bytes_get_serialized_size(&(pdl->bytes[i]));
    
    buf = pcalloc(dlsize);

    for ( i = 0; i < ndims ; i++ )
    {
        size_t bsize = 0;
        pc_bytes_serialize(&(pdl->bytes[i]), buf, &bsize);
        buf += bsize;
    }
    
    return buf;
}


PCDIMLIST *
pc_dimlist_deserialize(const PCSCHEMA *schema, int npoints, uint8_t *buf, int read_only, int flip_endian)
{
    int i;
    size_t size = 0;
    int ndims = schema->ndims;
    PCDIMLIST *pdl = pcalloc(sizeof(PCDIMLIST));
    pdl->schema = schema;
    pdl->npoints = npoints;
    pdl->bytes = pcalloc(ndims * sizeof(PCBYTES));
    
    for ( i = 0; i < ndims; i++ )
    {
        PCDIMENSION *dim = schema->dims[i];
        PCBYTES pcb = pdl->bytes[i];
        pc_bytes_deserialize(buf, dim, &pcb, read_only, flip_endian);
        /* pc_bytes_deserialize can't fill in npoints and interpretation */
        pcb.npoints = npoints;
        pcb.interpretation = dim->interpretation;
        /* move forward to next data area */
        buf += pcb.size;
        pdl->bytes[i] = pcb;
    }
    
    return pdl;
}










