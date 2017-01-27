/***********************************************************************
* pc_patch.c
*
*  Pointclound patch handling. Create, get and set values from the
*  basic PCPATCH structure.
*
*  PgSQL Pointcloud is free and open source software provided
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include <math.h>
#include <assert.h>
#include "pc_api_internal.h"

int
pc_patch_compute_extent(PCPATCH *pa)
{
	if ( ! pa ) return PC_FAILURE;
	switch ( pa->type )
	{
	case PC_NONE:
		return pc_patch_uncompressed_compute_extent((PCPATCH_UNCOMPRESSED*)pa);
	case PC_GHT:
		return pc_patch_ght_compute_extent((PCPATCH_GHT*)pa);
	case PC_DIMENSIONAL:
		return pc_patch_dimensional_compute_extent((PCPATCH_DIMENSIONAL*)pa);
	case PC_LAZPERF:
		return pc_patch_lazperf_compute_extent((PCPATCH_LAZPERF*)pa);
	}
	return PC_FAILURE;
}

/**
* Calculate or re-calculate statistics for a patch.
*/
int
pc_patch_compute_stats(PCPATCH *pa)
{
	if ( ! pa ) return PC_FAILURE;

	switch ( pa->type )
	{
	case PC_NONE:
		return pc_patch_uncompressed_compute_stats((PCPATCH_UNCOMPRESSED*)pa);

	case PC_DIMENSIONAL:
	{
		PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)pa);
		pc_patch_uncompressed_compute_stats(pu);
		pa->stats = pu->stats; pu->stats = NULL;
		pc_patch_uncompressed_free(pu);
		return PC_SUCCESS;
	}
	case PC_GHT:
	{
		PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_ght((PCPATCH_GHT*)pa);
		pc_patch_uncompressed_compute_stats(pu);
		pa->stats = pc_stats_clone(pu->stats);
		pc_patch_uncompressed_free(pu);
		return PC_SUCCESS;
	}
	case PC_LAZPERF:
	{
		PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_lazperf((PCPATCH_LAZPERF*)pa);
		pc_patch_uncompressed_compute_stats(pu);
		pa->stats = pc_stats_clone(pu->stats);
		pc_patch_uncompressed_free(pu);
		return PC_SUCCESS;
	}
	default:
	{
		pcerror("%s: unknown compression type", __func__, pa->type);
		return PC_FAILURE;
	}
	}

	return PC_FAILURE;
}

void
pc_patch_free(PCPATCH *patch)
{
	if ( patch->stats )
	{
		pc_stats_free( patch->stats );
		patch->stats = NULL;
	}

	switch( patch->type )
	{
	case PC_NONE:
	{
		pc_patch_uncompressed_free((PCPATCH_UNCOMPRESSED*)patch);
		break;
	}
	case PC_GHT:
	{
		pc_patch_ght_free((PCPATCH_GHT*)patch);
		break;
	}
	case PC_DIMENSIONAL:
	{
		pc_patch_dimensional_free((PCPATCH_DIMENSIONAL*)patch);
		break;
	}
	case PC_LAZPERF:
	{
		pc_patch_lazperf_free((PCPATCH_LAZPERF*)patch);
		break;
	}
	default:
	{
		pcerror("%s: unknown compression type %d", __func__, patch->type);
		break;
	}
	}
}


PCPATCH *
pc_patch_from_pointlist(const PCPOINTLIST *ptl)
{
	return (PCPATCH*)pc_patch_uncompressed_from_pointlist(ptl);
}


PCPATCH *
pc_patch_compress(const PCPATCH *patch, void *userdata)
{
	uint32_t schema_compression = patch->schema->compression;
	uint32_t patch_compression = patch->type;

	switch ( schema_compression )
	{
	case PC_DIMENSIONAL:
	{
		if ( patch_compression == PC_NONE )
		{
			/* Dimensionalize, dimensionally compress, return */
			PCPATCH_DIMENSIONAL *pcdu = pc_patch_dimensional_from_uncompressed((PCPATCH_UNCOMPRESSED*)patch);
			PCPATCH_DIMENSIONAL *pcdd = pc_patch_dimensional_compress(pcdu, (PCDIMSTATS*)userdata);
			pc_patch_dimensional_free(pcdu);
			return (PCPATCH*)pcdd;
		}
		else if ( patch_compression == PC_DIMENSIONAL )
		{
			/* Make sure it's compressed, return */
			return (PCPATCH*)pc_patch_dimensional_compress((PCPATCH_DIMENSIONAL*)patch, (PCDIMSTATS*)userdata);
		}
		else if ( patch_compression == PC_GHT )
		{
			/* Uncompress, dimensionalize, dimensionally compress, return */
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_ght((PCPATCH_GHT*)patch);
			PCPATCH_DIMENSIONAL *pcdu  = pc_patch_dimensional_from_uncompressed(pcu);
			PCPATCH_DIMENSIONAL *pcdc  = pc_patch_dimensional_compress(pcdu, NULL);
			pc_patch_dimensional_free(pcdu);
			return (PCPATCH*)pcdc;
		}
		else if ( patch_compression == PC_LAZPERF )
		{
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_lazperf( (PCPATCH_LAZPERF*) patch );
			PCPATCH_DIMENSIONAL *pal = pc_patch_dimensional_from_uncompressed( pcu );
			PCPATCH_DIMENSIONAL *palc = pc_patch_dimensional_compress( pal, NULL );
			pc_patch_dimensional_free(pal);
			return (PCPATCH*) palc;
		}
		else
		{
			pcerror("%s: unknown patch compression type %d", __func__, patch_compression);
		}
	}
	case PC_NONE:
	{
		if ( patch_compression == PC_NONE )
		{
			return (PCPATCH*)patch;
		}
		else if ( patch_compression == PC_DIMENSIONAL )
		{
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)patch);
			return (PCPATCH*)pcu;

		}
		else if ( patch_compression == PC_GHT )
		{
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_ght((PCPATCH_GHT*)patch);
			return (PCPATCH*)pcu;
		}
		else if ( patch_compression == PC_LAZPERF )
		{
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_lazperf( (PCPATCH_LAZPERF*)patch );
			return (PCPATCH*)pcu;
		}
		else
		{
			pcerror("%s: unknown patch compression type %d", __func__, patch_compression);
		}
	}
	case PC_GHT:
	{
		if ( patch_compression == PC_NONE )
		{
			PCPATCH_GHT *pgc = pc_patch_ght_from_uncompressed((PCPATCH_UNCOMPRESSED*)patch);
			if ( ! pgc ) pcerror("%s: ght compression failed", __func__);
			return (PCPATCH*)pgc;
		}
		else if ( patch_compression == PC_DIMENSIONAL )
		{
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)patch);
			PCPATCH_GHT *pgc = pc_patch_ght_from_uncompressed((PCPATCH_UNCOMPRESSED*)patch);
			pc_patch_uncompressed_free(pcu);
			return (PCPATCH*)pgc;
		}
		else if ( patch_compression == PC_GHT )
		{
			return (PCPATCH*)patch;
		}
		else if ( patch_compression == PC_LAZPERF )
		{
			PCPATCH_LAZPERF *pal = pc_patch_lazperf_from_uncompressed((PCPATCH_UNCOMPRESSED*)patch);
			return (PCPATCH*)pal;
		}
		else
		{
			pcerror("%s: unknown patch compression type %d", __func__, patch_compression);
		}
	}
	case PC_LAZPERF:
	{
		if ( patch_compression == PC_NONE )
		{
			PCPATCH_LAZPERF *pgc = pc_patch_lazperf_from_uncompressed((PCPATCH_UNCOMPRESSED*)patch);
			if ( ! pgc ) pcerror("%s: lazperf compression failed", __func__);
			return (PCPATCH*)pgc;
		}
		else if ( patch_compression == PC_DIMENSIONAL )
		{
			PCPATCH_UNCOMPRESSED *pad = pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)patch);
			PCPATCH_LAZPERF *pal = pc_patch_lazperf_from_uncompressed( (PCPATCH_UNCOMPRESSED*) pad );
			pc_patch_uncompressed_free( pad );
			return (PCPATCH*)pal;
		}
		else if ( patch_compression == PC_GHT )
		{
			PCPATCH_UNCOMPRESSED *pcu = pc_patch_uncompressed_from_ght((PCPATCH_GHT*)patch);
			PCPATCH_LAZPERF *pal = pc_patch_lazperf_from_uncompressed((PCPATCH_UNCOMPRESSED*)pcu);
			pc_patch_uncompressed_free(pcu);
			return (PCPATCH*)pal;
		}
		else if ( patch_compression == PC_LAZPERF )
		{
			return (PCPATCH*)patch;
		}
		else
		{
			pcerror("%s: unknown patch compression type %d", __func__, patch_compression);
		}
	}
	default:
	{
		pcerror("%s: unknown schema compression type %d", __func__, schema_compression);
	}
	}

	pcerror("%s: fatal error", __func__);
	return NULL;
}


PCPATCH *
pc_patch_uncompress(const PCPATCH *patch)
{
	uint32_t patch_compression = patch->type;

	if ( patch_compression == PC_DIMENSIONAL )
	{
		PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_dimensional((PCPATCH_DIMENSIONAL*)patch);
		return (PCPATCH*)pu;
	}

	if ( patch_compression == PC_NONE )
	{
		return (PCPATCH*)patch;
	}

	if ( patch_compression == PC_GHT )
	{
		PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_ght((PCPATCH_GHT*)patch);
		return (PCPATCH*)pu;
	}

	if ( patch_compression == PC_LAZPERF )
	{
		PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_lazperf( (PCPATCH_LAZPERF*)patch );
		return (PCPATCH*) pu;
	}

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
	PCPATCH *patch;

	if ( ! wkbsize )
	{
		pcerror("%s: zero length wkb", __func__);
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
		pcerror("%s: wkb pcid (%d) not consistent with schema pcid (%d)", __func__, pcid, s->pcid);
	}

	switch ( compression )
	{
	case PC_NONE:
	{
		patch = pc_patch_uncompressed_from_wkb(s, wkb, wkbsize);
		break;
	}
	case PC_DIMENSIONAL:
	{
		patch = pc_patch_dimensional_from_wkb(s, wkb, wkbsize);
		break;
	}
	case PC_GHT:
	{
		patch = pc_patch_ght_from_wkb(s, wkb, wkbsize);
		break;
	}
	case PC_LAZPERF:
	{
		patch = pc_patch_lazperf_from_wkb(s, wkb, wkbsize);
		break;
	}
	default:
	{
		/* Don't get here */
		pcerror("%s: unknown compression '%d' requested", __func__, compression);
		return NULL;
	}
	}

	if ( PC_FAILURE == pc_patch_compute_extent(patch) )
		pcerror("%s: pc_patch_compute_extent failed", __func__);

	if ( PC_FAILURE == pc_patch_compute_stats(patch) )
		pcerror("%s: pc_patch_compute_stats failed", __func__);

	return patch;

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
		return pc_patch_ght_to_wkb((PCPATCH_GHT*)patch, wkbsize);
	}
	case PC_LAZPERF:
	{
		return pc_patch_lazperf_to_wkb((PCPATCH_LAZPERF*)patch, wkbsize);
	}
	}
	pcerror("%s: unknown compression requested '%d'", __func__, patch->schema->compression);
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
	case PC_GHT:
		return pc_patch_ght_to_string((PCPATCH_GHT*)patch);
	case PC_LAZPERF:
		return pc_patch_lazperf_to_string( (PCPATCH_LAZPERF*)patch );
	}
	pcerror("%s: unsupported compression %d requested", __func__, patch->type);
	return NULL;
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
			pcerror("%s: inconsistent schemas in input", __func__);
			return NULL;
		}
		totalpoints += palist[i]->npoints;
	}

	/* Blank output */
	paout = pc_patch_uncompressed_make(schema, totalpoints);
	buf = paout->data;

	/* Uncompress dimensionals, copy uncompressed */
	for ( i = 0; i < numpatches; i++ )
	{
		const PCPATCH *pa = palist[i];

		/* Update bounds */
		pc_bounds_merge(&(paout->bounds), &(pa->bounds));

		switch ( pa->type )
		{
		case PC_DIMENSIONAL:
		{
			PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_dimensional((const PCPATCH_DIMENSIONAL*)pa);
			size_t sz = pu->schema->size * pu->npoints;
			memcpy(buf, pu->data, sz);
			buf += sz;
			pc_patch_free((PCPATCH*)pu);
			break;
		}
		case PC_GHT:
		{
			PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_ght((const PCPATCH_GHT*)pa);
			size_t sz = pu->schema->size * pu->npoints;
			memcpy(buf, pu->data, sz);
			buf += sz;
			pc_patch_uncompressed_free(pu);
			break;
		}
		case PC_NONE:
		{
			PCPATCH_UNCOMPRESSED *pu = (PCPATCH_UNCOMPRESSED*)pa;
			size_t sz = pu->schema->size * pu->npoints;
			memcpy(buf, pu->data, sz);
			buf += sz;
			break;
		}
		case PC_LAZPERF:
		{
			PCPATCH_UNCOMPRESSED *pu = pc_patch_uncompressed_from_lazperf((const PCPATCH_LAZPERF*)pa);
			size_t sz = pu->schema->size * pu->npoints;
			memcpy(buf, pu->data, sz);
			buf += sz;
			pc_patch_uncompressed_free(pu);
			break;
		}
		default:
		{
			pcerror("%s: unknown compression type (%d)", __func__, pa->type);
			break;
		}
		}
	}

	paout->npoints = totalpoints;

	if ( PC_FAILURE == pc_patch_uncompressed_compute_stats(paout) )
	{
		pcerror("%s: stats computation failed", __func__);
		return NULL;
	}

	return (PCPATCH*)paout;
}

/** get point n from patch */
/** positive 1-based:  1=first point,  npoints=last  point */
/** negative 1-based: -1=last  point, -npoints=first point */
PCPOINT *pc_patch_pointn(const PCPATCH *patch, int n)
{
    if(!patch) return NULL;
    if(n<0) n = patch->npoints+n; // negative indices count a backward
    else --n; // 1-based => 0-based indexing
    if(n<0 || n>= patch->npoints) return NULL;

    switch( patch->type )
    {
    case PC_NONE:
        return pc_patch_uncompressed_pointn((PCPATCH_UNCOMPRESSED*)patch,n);
    case PC_DIMENSIONAL:
        return pc_patch_dimensional_pointn((PCPATCH_DIMENSIONAL*)patch,n);
    case PC_GHT:
        return pc_patch_ght_pointn((PCPATCH_GHT*)patch,n);
    }
    pcerror("%s: unsupported compression %d requested", __func__, patch->type);
    return NULL;
}
