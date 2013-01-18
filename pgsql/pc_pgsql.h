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

/* UTILITY FUNCTIONS */

PCSCHEMA* pc_get_schema_by_id(uint32_t pcid);
