/***********************************************************************
* pc_pgsql.h
*
*  Common header file for all PgSQL pointcloud functions.
*
*  PgSQL Pointcloud is free and open source software provided
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include "pc_api.h"

#include "postgres.h"
#include "utils/elog.h"

/* Try to move these down */
#include "utils/array.h"
#include "utils/builtins.h"  /* for pg_atoi */
#include "lib/stringinfo.h"  /* For binary input */
#include "catalog/pg_type.h" /* for CSTRINGOID */

#define POINTCLOUD_FORMATS "pointcloud_formats"
#define POINTCLOUD_FORMATS_XML "schema"
#define POINTCLOUD_FORMATS_SRID "srid"

#define PG_GETARG_SERPOINT_P(argnum) (SERIALIZED_POINT*)PG_DETOAST_DATUM(PG_GETARG_DATUM(argnum))
#define PG_GETARG_SERPATCH_P(argnum) (SERIALIZED_PATCH*)PG_DETOAST_DATUM(PG_GETARG_DATUM(argnum))

#define PG_GETHEADER_SERPATCH_P(argnum) (SERIALIZED_PATCH*)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(argnum), 0, sizeof(SERIALIZED_PATCH))

#define PG_GETHEADERX_SERPATCH_P(argnum, extra) (SERIALIZED_PATCH*)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(argnum), 0, sizeof(SERIALIZED_PATCH)+extra)

#define PG_GETHEADER_STATS_P(argnum, statsize) (uint8_t*)(((SERIALIZED_PATCH*)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(argnum), 0, sizeof(SERIALIZED_PATCH) + statsize))->data)

#define AUTOCOMPRESS_NO 0
#define AUTOCOMPRESS_YES 1

/*
 * SERIALIZED_POINT, SERIALIZED_PATCH, and the serialize/deserialize
 * functions are now declared in pc_api.h (included above) so that
 * libpc exposes them to non-PostgreSQL consumers.
 */

/* PGSQL / POINTCLOUD UTILITY FUNCTIONS */
uint32 pcid_from_typmod(const int32 typmod);

/** Look-up the PCID in the POINTCLOUD_FORMATS table, and construct a PC_SCHEMA from the XML therein */
PCSCHEMA* pc_schema_from_pcid(uint32_t pcid, FunctionCallInfoData *fcinfo);

/** Look-up the PCID in the POINTCLOUD_FORMATS table, and construct a PC_SCHEMA from the XML therein */
PCSCHEMA* pc_schema_from_pcid_uncached(uint32 pcid);

/** Create a new readwrite PCPOINT from a hex string */
PCPOINT* pc_point_from_hexwkb(const char *hexwkb, size_t hexlen, FunctionCallInfoData *fcinfo);

/** Create a hex representation of a PCPOINT */
char* pc_point_to_hexwkb(const PCPOINT *pt);

/** Create a new readwrite PCPATCH from a hex string */
PCPATCH* pc_patch_from_hexwkb(const char *hexwkb, size_t hexlen, FunctionCallInfoData *fcinfo);

/** Create a hex representation of a PCPATCH */
char* pc_patch_to_hexwkb(const PCPATCH *patch);

/** Returns OGC WKB for envelope of PCPATCH */
uint8_t* pc_patch_to_geometry_wkb_envelope(const SERIALIZED_PATCH *pa, const PCSCHEMA *schema, size_t *wkbsize);

/** Read the first few bytes off an object to get the datum */
uint32 pcid_from_datum(Datum d);
