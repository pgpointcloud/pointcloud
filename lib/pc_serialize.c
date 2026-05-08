/***********************************************************************
* pc_serialize.c
*
*  Serialization / deserialization between in-memory PCPOINT/PCPATCH
*  structs and their varlena on-disk representations.
*
*  Previously these functions lived in pgsql/pc_pgsql.c and were only
*  reachable via the PostgreSQL extension shared object.  Moving them
*  here makes them part of libpc.a so that non-PostgreSQL consumers
*  (e.g. MEOS) can call them without loading the PG extension.
*
*  PgSQL Pointcloud is free and open source software provided
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include "pc_api.h"
#include "pc_api_internal.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

/*
 * When building libpc without PostgreSQL headers, provide minimal
 * varlena-layout helpers that are bit-for-bit compatible with PG's
 * SET_VARSIZE / VARSIZE for non-toasted, non-compressed objects.
 */
#ifndef SET_VARSIZE
#define SET_VARSIZE(ptr, sz) (*(uint32_t *)(ptr) = (uint32_t)(sz))
#define VARSIZE(ptr)         ((size_t)(*(uint32_t *)(ptr) & 0x3FFFFFFF))
#endif


/**********************************************************************************
* SERIALIZATION/DESERIALIZATION UTILITIES
*/

SERIALIZED_POINT *
pc_point_serialize(const PCPOINT *pcpt)
{
	size_t serpt_size = sizeof(SERIALIZED_POINT) - 1 + pcpt->schema->size;
	SERIALIZED_POINT *serpt = pcalloc(serpt_size);
	serpt->pcid = pcpt->schema->pcid;
	memcpy(serpt->data, pcpt->data, pcpt->schema->size);
	SET_VARSIZE(serpt, serpt_size);
	return serpt;
}

PCPOINT *
pc_point_deserialize(const SERIALIZED_POINT *serpt, const PCSCHEMA *schema)
{
	PCPOINT *pcpt;
	size_t pgsize = VARSIZE(serpt) + 1 - sizeof(SERIALIZED_POINT);
	/*
	* Big problem, the size on disk doesn't match what we expect,
	* so we cannot reliably interpret the contents.
	*/
	if ( schema->size != pgsize )
	{
		pcerror("schema size and disk size mismatch, repair the schema");
		return NULL;
	}
	pcpt = pc_point_from_data(schema, serpt->data);
	return pcpt;
}


size_t
pc_patch_serialized_size(const PCPATCH *patch)
{
	size_t stats_size = pc_stats_size(patch->schema);
	size_t common_size = sizeof(SERIALIZED_PATCH) - 1;
	switch( patch->type )
	{
	case PC_NONE:
	{
		PCPATCH_UNCOMPRESSED *pu = (PCPATCH_UNCOMPRESSED*)patch;
		return common_size + stats_size + pu->datasize;
	}
	case PC_GHT:
	{
		static size_t ghtsize_size = 4;
		PCPATCH_GHT *pg = (PCPATCH_GHT*)patch;
		return common_size + stats_size + ghtsize_size + pg->ghtsize;
	}
	case PC_DIMENSIONAL:
	{
		return common_size + stats_size + pc_patch_dimensional_serialized_size((PCPATCH_DIMENSIONAL*)patch);
	}
	default:
	{
		pcerror("%s: unknown compresed %d", __func__, patch->type);
	}
	}
	return -1;
}

static size_t
pc_patch_stats_serialize(uint8_t *buf, const PCSCHEMA *schema, const PCSTATS *stats)
{
	size_t sz = schema->size;
	/* Copy min */
	memcpy(buf, stats->min.data, sz);
	/* Copy max */
	memcpy(buf + sz, stats->max.data, sz);
	/* Copy avg */
	memcpy(buf + 2*sz, stats->avg.data, sz);

	return sz*3;
}

/**
* Stats are always three PCPOINT serializations in a row,
* min, max, avg. Their size is the uncompressed buffer size for
* a point, the schema->size.
*/
PCSTATS *
pc_patch_stats_deserialize(const PCSCHEMA *schema, const uint8_t *buf)
{
	size_t sz = schema->size;
	const uint8_t *buf_min = buf;
	const uint8_t *buf_max = buf + sz;
	const uint8_t *buf_avg = buf + 2*sz;

	return pc_stats_new_from_data(schema, buf_min, buf_max, buf_avg);
}

static SERIALIZED_PATCH *
pc_patch_dimensional_serialize(const PCPATCH *patch_in)
{
	int i;
	uint8_t *buf;
	size_t serpch_size = pc_patch_serialized_size(patch_in);
	SERIALIZED_PATCH *serpch = pcalloc(serpch_size);
	const PCPATCH_DIMENSIONAL *patch = (PCPATCH_DIMENSIONAL*)patch_in;

	assert(patch_in);
	assert(patch_in->type == PC_DIMENSIONAL);

	/* Copy basics */
	serpch->pcid = patch->schema->pcid;
	serpch->npoints = patch->npoints;
	serpch->bounds = patch->bounds;
	serpch->compression = patch->type;

	/* Get a pointer to the data area */
	buf = serpch->data;

	/* Write stats into the buffer */
	if ( patch->stats )
	{
		buf += pc_patch_stats_serialize(buf, patch->schema, patch->stats);
	}
	else
	{
		pcerror("%s: stats missing!", __func__);
	}

	/* Write each dimension in after the stats */
	for ( i = 0; i < patch->schema->ndims; i++ )
	{
		size_t bsize = 0;
		PCBYTES *pcb = &(patch->bytes[i]);
		pc_bytes_serialize(pcb, buf, &bsize);
		buf += bsize;
	}

	SET_VARSIZE(serpch, serpch_size);
	return serpch;
}


static SERIALIZED_PATCH *
pc_patch_ght_serialize(const PCPATCH *patch_in)
{
	size_t serpch_size = pc_patch_serialized_size(patch_in);
	SERIALIZED_PATCH *serpch = pcalloc(serpch_size);
	const PCPATCH_GHT *patch = (PCPATCH_GHT*)patch_in;
	uint32_t ghtsize = patch->ghtsize;
	uint8_t *buf = serpch->data;

	assert(patch);
	assert(patch->type == PC_GHT);

	/* Copy basics */
	serpch->pcid = patch->schema->pcid;
	serpch->npoints = patch->npoints;
	serpch->bounds = patch->bounds;
	serpch->compression = patch->type;

	/* Write stats into the buffer first */
	if ( patch->stats )
	{
		buf += pc_patch_stats_serialize(buf, patch->schema, patch->stats);
	}
	else
	{
		pcerror("%s: stats missing!", __func__);
	}

	/* Write tree buffer size */
	memcpy(buf, &(ghtsize), 4);
	buf += 4;

	/* Write tree buffer */
	memcpy(buf, patch->ght, patch->ghtsize);
	SET_VARSIZE(serpch, serpch_size);
	return serpch;
}

static SERIALIZED_PATCH *
pc_patch_uncompressed_serialize(const PCPATCH *patch_in)
{
	uint8_t *buf;
	size_t serpch_size;
	SERIALIZED_PATCH *serpch;
	const PCPATCH_UNCOMPRESSED *patch = (PCPATCH_UNCOMPRESSED *)patch_in;

	serpch_size = pc_patch_serialized_size(patch_in);
	serpch = pcalloc(serpch_size);

	/* Copy basic */
	serpch->compression = patch->type;
	serpch->pcid = patch->schema->pcid;
	serpch->npoints = patch->npoints;
	serpch->bounds = patch->bounds;

	/* Write stats into the buffer first */
	buf = serpch->data;
	if ( patch->stats )
	{
		buf += pc_patch_stats_serialize(buf, patch->schema, patch->stats);
	}
	else
	{
		pcerror("%s: stats missing!", __func__);
	}

	/* Copy point list into data buffer */
	memcpy(buf, patch->data, patch->datasize);
	SET_VARSIZE(serpch, serpch_size);
	return serpch;
}


/**
* Convert struct to byte array.
* Userdata is currently only PCDIMSTATS, hopefully updated across
* a number of iterations and saved.
*/
SERIALIZED_PATCH *
pc_patch_serialize(const PCPATCH *patch_in, void *userdata)
{
	PCPATCH *patch = (PCPATCH*)patch_in;
	SERIALIZED_PATCH *serpatch = NULL;
	/*
	* Ensure the patch has stats calculated before going on
	*/
	if ( ! patch->stats )
	{
		pcerror("%s: patch is missing stats", __func__);
		return NULL;
	}
	/*
	* Convert the patch to the final target compression,
	* which is the one in the schema.
	*/
	if ( patch->type != patch->schema->compression )
	{
		patch = pc_patch_compress(patch_in, userdata);
	}

	switch( patch->type )
	{
	case PC_NONE:
	{
		serpatch = pc_patch_uncompressed_serialize(patch);
		break;
	}
	case PC_DIMENSIONAL:
	{
		serpatch = pc_patch_dimensional_serialize(patch);
		break;
	}
	case PC_GHT:
	{
		serpatch = pc_patch_ght_serialize(patch);
		break;
	}
	default:
	{
		pcerror("%s: unsupported compression type %d", __func__, patch->type);
	}
	}

	if ( patch != patch_in )
		pc_patch_free(patch);

	return serpatch;
}


/**
* Convert struct to byte array.
* Userdata is currently only PCDIMSTATS, hopefully updated across
* a number of iterations and saved.
*/
SERIALIZED_PATCH *
pc_patch_serialize_to_uncompressed(const PCPATCH *patch_in)
{
	PCPATCH *patch = (PCPATCH*)patch_in;
	SERIALIZED_PATCH *serpatch;

	/*  Convert the patch to uncompressed, if necessary */
	if ( patch->type != PC_NONE )
	{
		patch = pc_patch_uncompress(patch_in);
	}

	serpatch = pc_patch_uncompressed_serialize(patch);

	/* An uncompressed input won't result in a copy */
	if ( patch != patch_in )
		pc_patch_free(patch);

	return serpatch;
}

static PCPATCH *
pc_patch_uncompressed_deserialize(const SERIALIZED_PATCH *serpatch, const PCSCHEMA *schema)
{
	uint8_t *buf;
	size_t stats_size = pc_stats_size(schema); /* 3 pcpoints worth of stats */
	PCPATCH_UNCOMPRESSED *patch = pcalloc(sizeof(PCPATCH_UNCOMPRESSED));

	/* Set up basic info */
	patch->type = serpatch->compression;
	patch->schema = schema;
	patch->readonly = true;
	patch->npoints = serpatch->npoints;
	patch->maxpoints = 0;
	patch->bounds = serpatch->bounds;

	buf = (uint8_t*)serpatch->data;

	/* Point into the stats area */
	patch->stats = pc_patch_stats_deserialize(schema, buf);

	/* Advance data pointer past the stats serialization */
	patch->data = buf + stats_size;

	/* Calculate the point data buffer size */
	patch->datasize = VARSIZE(serpatch) - sizeof(SERIALIZED_PATCH) + 1 - stats_size;
	if ( patch->datasize != patch->npoints * schema->size )
		pcerror("%s: calucated patch data sizes don't match (%d != %d)", __func__, patch->datasize, patch->npoints * schema->size);

	return (PCPATCH*)patch;
}

static PCPATCH *
pc_patch_dimensional_deserialize(const SERIALIZED_PATCH *serpatch, const PCSCHEMA *schema)
{
	PCPATCH_DIMENSIONAL *patch;
	int i;
	const uint8_t *buf;
	int ndims = schema->ndims;
	int npoints = serpatch->npoints;
	size_t stats_size = pc_stats_size(schema); /* 3 pcpoints worth of stats */

	/* Reference the external data */
	patch = pcalloc(sizeof(PCPATCH_DIMENSIONAL));

	/* Set up basic info */
	patch->type = serpatch->compression;
	patch->schema = schema;
	patch->readonly = true;
	patch->npoints = npoints;
	patch->bounds = serpatch->bounds;

	/* Point into the stats area */
	patch->stats = pc_patch_stats_deserialize(schema, serpatch->data);

	/* Set up dimensions */
	patch->bytes = pcalloc(ndims * sizeof(PCBYTES));
	buf = serpatch->data + stats_size;

	for ( i = 0; i < ndims; i++ )
	{
		PCBYTES *pcb = &(patch->bytes[i]);
		PCDIMENSION *dim = schema->dims[i];
		pc_bytes_deserialize(buf, dim, pcb, true /*readonly*/, false /*flipendian*/);
		pcb->npoints = npoints;
		buf += pc_bytes_serialized_size(pcb);
	}

	return (PCPATCH*)patch;
}

/*
* We don't do any radical deserialization here. Don't build out the tree, just
* set up pointers to the start of the buffer, so we can build it out later
* if necessary.
*/
static PCPATCH *
pc_patch_ght_deserialize(const SERIALIZED_PATCH *serpatch, const PCSCHEMA *schema)
{
	PCPATCH_GHT *patch;
	uint32_t ghtsize;
	int npoints = serpatch->npoints;
	size_t stats_size = pc_stats_size(schema); /* 3 pcpoints worth of stats */
	uint8_t *buf = (uint8_t*)serpatch->data + stats_size;

	/* Reference the external data */
	patch = pcalloc(sizeof(PCPATCH_GHT));

	/* Set up basic info */
	patch->type = serpatch->compression;
	patch->schema = schema;
	patch->readonly = true;
	patch->npoints = npoints;
	patch->bounds = serpatch->bounds;

	/* Point into the stats area */
	patch->stats = pc_patch_stats_deserialize(schema, serpatch->data);

	/* Set up ght buffer */
	memcpy(&ghtsize, buf, 4);
	patch->ghtsize = ghtsize;
	patch->ght = buf + 4;

	/* That's it */
	return (PCPATCH*)patch;
}

PCPATCH *
pc_patch_deserialize(const SERIALIZED_PATCH *serpatch, const PCSCHEMA *schema)
{
	switch(serpatch->compression)
	{
	case PC_NONE:
		return pc_patch_uncompressed_deserialize(serpatch, schema);
	case PC_DIMENSIONAL:
		return pc_patch_dimensional_deserialize(serpatch, schema);
	case PC_GHT:
		return pc_patch_ght_deserialize(serpatch, schema);
	}
	pcerror("%s: unsupported compression type", __func__);
	return NULL;
}
