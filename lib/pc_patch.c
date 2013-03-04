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
#include <assert.h>
#include "pc_api_internal.h"
#include "stringbuffer.h"





int
pc_patch_compute_extent(PCPATCH *patch)
{
	switch( patch->type )
	{
		case PC_NONE:
			return pc_patch_uncompressed_compute_extent((PCPATCH_UNCOMPRESSED*)patch);
		case PC_GHT:
			return PC_FAILURE;
		case PC_DIMENSIONAL:
			return pc_patch_dimensional_compute_extent((PCPATCH_DIMENSIONAL*)patch);
	}
	return PC_FAILURE;
}

void
pc_patch_free(PCPATCH *patch)
{
	switch( patch->type )
	{
		case PC_NONE:
		{
			pc_patch_uncompressed_free((PCPATCH_UNCOMPRESSED*)patch);
            break;
		}	
		case PC_GHT:
	    {
			pcerror("pc_patch_free: GHT not supported");
            break;
		}
		case PC_DIMENSIONAL:
		{
			pc_patch_dimensional_free((PCPATCH_DIMENSIONAL*)patch);
            break;
		}
		default:
		{
        	pcerror("pc_patch_free: unknown compression type %d", patch->type);
            break;
	    }
	}
}


PCPATCH * 
pc_patch_from_pointlist(const PCPOINTLIST *ptl)
{
    return (PCPATCH*)pc_patch_uncompressed_from_pointlist(ptl);
}

PCPOINTLIST * 
pc_patch_to_pointlist(const PCPATCH *patch)
{
	switch ( patch->type )
	{
		case PC_NONE:
		{
			return pc_patch_uncompressed_to_pointlist((PCPATCH_UNCOMPRESSED*)patch);
		}
		case PC_GHT:
		{
			// return pc_patch_to_pointlist_ght(patch);
		}
		case PC_DIMENSIONAL:
		{
            PCPATCH_UNCOMPRESSED *pch = pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)patch);
			PCPOINTLIST *ptl = pc_patch_uncompressed_to_pointlist((PCPATCH_UNCOMPRESSED*)patch);
            pc_patch_uncompressed_free(pch);
            return ptl;
		}
	}
	
	/* Don't get here */
	pcerror("pc_patch_to_pointlist: unsupported compression type %d", patch->type);
	return NULL;
}

PCPATCH *
pc_patch_compress(const PCPATCH *patch, void *userdata)
{
	uint32_t schema_compression = patch->schema->compression;
	uint32_t patch_compression = patch->type;
	
    if ( schema_compression == PC_DIMENSIONAL && 
          patch_compression == PC_NONE )
    {
        PCPATCH_DIMENSIONAL *pcdu = pc_patch_dimensional_from_uncompressed((PCPATCH_UNCOMPRESSED*)patch);
        PCPATCH_DIMENSIONAL *pcdd = pc_patch_dimensional_compress(pcdu, (PCDIMSTATS*)userdata);
        pc_patch_dimensional_free(pcdu);
        return (PCPATCH*)pcdd;
    }
    
    if ( schema_compression == PC_DIMENSIONAL && 
          patch_compression == PC_DIMENSIONAL )
    {
        return (PCPATCH*)pc_patch_dimensional_compress((PCPATCH_DIMENSIONAL*)patch, (PCDIMSTATS*)userdata);
    }
    
    if ( schema_compression == PC_NONE && 
          patch_compression == PC_NONE )
    {
        return (PCPATCH*)patch;
    }

    pcerror("pc_patch_compress: cannot convert patch compressed %d to compressed %d", patch_compression, schema_compression);
    return NULL;
}


PCPATCH * 
pc_patch_from_wkb(const PCSCHEMA *s, uint8_t *wkb, size_t wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uchar[]:  data (interpret relative to pcid and compression)
	*/
	uint32_t compression, pcid;
	
	if ( ! wkbsize )
	{
		pcerror("pc_patch_from_wkb: zero length wkb");
	}
	
	/*
	* It is possible for the WKB compression to be different from the
	* schema compression at this point. The schema compression is only
	* forced at serialization time.
	*/
	pcid = wkb_get_pcid(wkb);
	compression = wkb_get_compression(wkb);

	if ( pcid != s->pcid )
	{
		pcerror("pc_patch_from_wkb: wkb pcid (%d) not consistent with schema pcid (%d)", pcid, s->pcid);
	}
	
	switch ( compression )
	{
		case PC_NONE:
		{
			return pc_patch_uncompressed_from_wkb(s, wkb, wkbsize);
		}
		case PC_DIMENSIONAL:
		{
			return pc_patch_dimensional_from_wkb(s, wkb, wkbsize);
		}
		case PC_GHT:
		{
			pcerror("pc_patch_from_wkb: GHT compression not yet supported");
			return NULL;
		}
	}
	
	/* Don't get here */
	pcerror("pc_patch_from_wkb: unknown compression '%d' requested", compression);
	return NULL;
}



uint8_t *
pc_patch_to_wkb(const PCPATCH *patch, size_t *wkbsize)
{
	/*
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uchar[]:  data (interpret relative to pcid and compression)
	*/
	switch ( patch->type )
	{
		case PC_NONE:
		{
			return pc_patch_uncompressed_to_wkb((PCPATCH_UNCOMPRESSED*)patch, wkbsize);
		}
		case PC_DIMENSIONAL:
		{
			return pc_patch_dimensional_to_wkb((PCPATCH_DIMENSIONAL*)patch, wkbsize);
		}
		case PC_GHT:
		{
			pcerror("pc_patch_to_wkb: GHT compression not yet supported");
			return NULL;
		}
	}	
	pcerror("pc_patch_to_wkb: unknown compression requested '%d'", patch->schema->compression);
	return NULL;
}

char *
pc_patch_to_string(const PCPATCH *patch)
{
    switch( patch->type )
    {
        case PC_NONE:
            return pc_patch_uncompressed_to_string((PCPATCH_UNCOMPRESSED*)patch);
        case PC_DIMENSIONAL:
            return pc_patch_dimensional_to_string((PCPATCH_DIMENSIONAL*)patch);
    }
    pcerror("pc_patch_to_string: unsupported compression %d requested", patch->type);
}

static uint8_t *
pc_patch_wkb_set_double(uint8_t *wkb, double d)
{
	memcpy(wkb, &d, 8);
	wkb += 8;
	return wkb;
}

static uint8_t *
pc_patch_wkb_set_int32(uint8_t *wkb, uint32_t i)
{
	memcpy(wkb, &i, 8);
	wkb += 4;
	return wkb;
}

static uint8_t *
pc_patch_wkb_set_char(uint8_t *wkb, char c)
{
	memcpy(wkb, &c, 1);
	wkb += 1;
	return wkb;
}

uint8_t *
pc_patch_to_geometry_wkb_envelope(const PCPATCH *pa, size_t *wkbsize)
{
	static uint32_t srid_mask = 0x20000000;
	static uint32_t nrings = 1;
	static uint32_t npoints = 5;
	uint32_t wkbtype = 3; /* WKB POLYGON */
	uint8_t *wkb, *ptr;
	int has_srid = PC_FALSE;
	size_t size = 1 + 4 + 4 + 4 + 2*npoints*8; /* endian + type + nrings + npoints + 5 dbl pts */
	double x, y;
	
	if ( pa->schema->srid > 0 )
	{
		has_srid = PC_TRUE;
		wkbtype |= srid_mask;
		size += 4;
	}

	wkb = pcalloc(size);
	ptr = wkb;
	
	ptr = pc_patch_wkb_set_char(ptr, machine_endian()); /* Endian flag */
	
	ptr = pc_patch_wkb_set_int32(ptr, wkbtype); /* TYPE = Polygon */
	
	if ( has_srid )
	{
		ptr = pc_patch_wkb_set_int32(ptr, pa->schema->srid); /* SRID */
	}
	
	ptr = pc_patch_wkb_set_int32(ptr, nrings);  /* NRINGS = 1 */
	ptr = pc_patch_wkb_set_int32(ptr, npoints); /* NPOINTS = 5 */
	
	/* Point 0 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmin);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymin);
	
	/* Point 1 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmin);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymax);
	
	/* Point 2 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmax);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymax);
	
	/* Point 3 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmax);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymin);
	
	/* Point 4 */
	ptr = pc_patch_wkb_set_double(ptr, pa->xmin);
	ptr = pc_patch_wkb_set_double(ptr, pa->ymin);
	
	if ( wkbsize ) *wkbsize = size;
	return wkb;
}

PCPATCH *
pc_patch_from_patchlist(PCPATCH **palist, int numpatches)
{
    int i;
    uint32_t totalpoints = 0;
    PCPATCH_UNCOMPRESSED *paout;
    const PCSCHEMA *schema = NULL;
    uint8_t *buf;
    
    assert(palist);
    assert(numpatches);
    
    /* All schemas better be the same... */
    schema = palist[0]->schema;
    
    /* How many points will this output have? */
    for ( i = 0; i < numpatches; i++ )
    {
        if ( schema->pcid != palist[i]->schema->pcid )
        {
            pcerror("pc_patch_from_patchlist: inconsistent schemas in input");
            return NULL;
        }
        totalpoints += palist[i]->npoints;
    }
    
    /* Blank output */
    paout = pc_patch_uncompressed_make(schema, totalpoints);
    buf = paout->data;
    paout->xmin = paout->ymin = MAXFLOAT;
    paout->xmax = paout->ymax = -1 * MAXFLOAT;
    
    /* Uncompress dimensionals, copy uncompressed */
    for ( i = 0; i < numpatches; i++ )
    {
        const PCPATCH *pa = palist[i];

        /* Update bounds */
        if ( pa->xmin < paout->xmin ) paout->xmin = pa->xmin;
        if ( pa->ymin < paout->ymin ) paout->ymin = pa->ymin;
        if ( pa->xmax > paout->xmax ) paout->xmax = pa->xmax;
        if ( pa->ymax > paout->ymax ) paout->ymax = pa->ymax;
        
        switch ( pa->type )
        {
            case PC_DIMENSIONAL:
            {
                PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_dimensional((const PCPATCH_DIMENSIONAL*)pa);
                size_t sz = schema->size * pu->npoints;
                memcpy(buf, pu->data, sz);
                buf += sz;
                pc_patch_uncompressed_free(pu);
                break;
            }
            case PC_GHT:
            {
                pcerror("pc_patch_from_patchlist: GHT compression not yet supported");
                break;
            }
            case PC_NONE:
            {
                PCPATCH_UNCOMPRESSED *pu = (PCPATCH_UNCOMPRESSED*)pa;
                size_t sz = schema->size * pu->npoints;
                memcpy(buf, pu->data, sz);
                buf += sz;
                break;
            }
            default:
            {
                pcerror("pc_patch_from_patchlist: unknown compresseion type", pa->type);
                break;
            }
        }
    }
    
    paout->npoints = totalpoints;
    return (PCPATCH*)paout;
}