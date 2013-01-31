/***********************************************************************
* pc_pgsql.h
*
*  Common header file for all PgSQL pointcloud functions.
*
* Portions Copyright (c) 2012, OpenGeo
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

#define PG_GETARG_SERPOINT_P(datum) (SERIALIZED_POINT*)PG_DETOAST_DATUM(PG_GETARG_DATUM(datum))
#define PG_GETARG_SERPATCH_P(datum) (SERIALIZED_PATCH*)PG_DETOAST_DATUM(PG_GETARG_DATUM(datum))

/**
* Serialized point type for clouds. Variable length, because there can be
* an arbitrary number of dimensions. The pcid is a foreign key
* reference to the POINTCLOUD_SCHEMAS table, where
* the underlying structure of the data is described in XML,
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct 
{
	uint32_t size;
	uint32_t pcid;
	uint8_t data[1];
}
SERIALIZED_POINT;

/**
* PgSQL patch type (collection of points) for clouds.
* Variable length, because there can be
* an arbitrary number of points encoded within.
* The pcid is a foriegn key reference to the
* POINTCLOUD_SCHEMAS table, where
* the underlying structure of the data is described in XML,
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct 
{
	uint32_t size;
	uint32_t pcid;
	uint32_t npoints;
	double xmin, xmax, ymin, ymax;
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

/** Create a new readwrite PCPOINT from a hex string */
PCPOINT* pc_point_from_hexwkb(const char *hexwkb, size_t hexlen);

/** Create a hex representation of a PCPOINT */
char* pc_point_to_hexwkb(const PCPOINT *pt);


/** Turn a PCPATCH into a byte buffer suitable for saving in PgSQL */
SERIALIZED_PATCH* pc_patch_serialize(const PCPATCH *patch);

/** Turn a byte buffer into a PCPATCH for processing */
PCPATCH* pc_patch_deserialize(const SERIALIZED_PATCH *serpatch);

/** Create a new readwrite PCPATCH from a hex string */
PCPATCH* pc_patch_from_hexwkb(const char *hexwkb, size_t hexlen);

/** Create a hex representation of a PCPOINT */
char* pc_patch_to_hexwkb(const PCPATCH *patch);
