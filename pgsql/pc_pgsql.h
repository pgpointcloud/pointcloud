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

#define PG_GETARG_SERPOINT_P(datum) (SERIALIZED_POINT*)PG_DETOAST_DATUM(PG_GETARG_DATUM(datum))

typedef struct 
{
	uint32_t size;
	uint32_t pcid;
	uint8_t data[1];
}
SERIALIZED_POINT;

typedef struct 
{
	uint32_t size;
	uint32_t pcid;
	uint32_t npoints;
	uint8_t data[1];
}
SERIALIZED_PATCH;

/* PGSQL / POINTCLOUD UTILITY FUNCTIONS */

/** Look-up the PCID in the POINTCLOUD_FORMATS table, and construct a PC_SCHEMA from the XML therein */
PCSCHEMA* pc_schema_get_by_id(uint32_t pcid);

/** Turn a PCPOINT into a byte buffer suitable for saving in PgSQL */
SERIALIZED_POINT* pc_point_serialize(const PCPOINT *pcpt);

/** Turn a byte buffer into a PCPOINT for processing */
PCPOINT* pc_point_deserialize(const SERIALIZED_POINT *serpt);

/** Returns 1 for little (NDR) and 0 for big (XDR) */
char machine_endian(void);

/** Turn a binary buffer into a hex string */
char* hexbytes_from_bytes(const uint8_t *bytebuf, size_t bytesize);

/** Create a new readwrite PCPOINT from a hex byte array */
PCPOINT* pc_point_from_wkb(uint8_t *wkb, size_t wkbsize);

/** Create a new readwrite PCPOINT from a hex string */
PCPOINT* pc_point_from_hexwkb(const char *hexwkb, size_t hexlen);

/** Returns serialized form of point */
uint8_t* wkb_from_point(const PCPOINT *pt, size_t *wkbsize);

