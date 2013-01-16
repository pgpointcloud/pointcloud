/***********************************************************************
* pc_api.h
*
*  Structures and function signatures for point clouds
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#ifndef _PC_API_H
#define _PC_API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hashtable.h"

/**********************************************************************
* DATA STRUCTURES
*/

/**
* Utility defines
*/
#define PC_TRUE 1
#define PC_FALSE 0
#define PC_SUCCESS 1
#define PC_FAILURE 0

/**
* How many compression types do we support?
*/
#define PCCOMPRESSIONTYPES 2

/**
* Compression types for PCPOINTS in a PCPATCH
*/
enum COMPRESSIONS
{
	PC_NONE = 0,
	PC_GHT = 1
};

/**
* Flags of endianness for inter-architecture
* data transfers.
*/
enum ENDIANS
{
	PC_NDR, /* Little */
	PC_XDR  /* Big */
};

/**
* Interpretation types for our dimension descriptions
*/
#define NUM_INTERPRETATIONS 11

enum INTERPRETATIONS
{
	PC_UNKNOWN = 0,
	PC_INT8,   PC_UINT8,
	PC_INT16,  PC_UINT16,
	PC_INT32,  PC_UINT32,
	PC_INT64,  PC_UINT64,
	PC_DOUBLE, PC_FLOAT
};

static char *INTERPRETATION_STRINGS[NUM_INTERPRETATIONS] =
{
	"unknown",
	"int8_t",  "uint8_t",
	"int16_t", "uint16_t",
	"int32_t", "uint32_t",
	"int64_t", "uint64_t",
	"double",  "float"
};

static size_t INTERPRETATION_SIZES[NUM_INTERPRETATIONS] =
{
	-1,    /* PC_UNKNOWN */
	1, 1,  /* PC_INT8, PC_UINT8, */
	2, 2,  /* PC_INT16, PC_UINT16 */
	4, 4,  /* PC_INT32, PC_UINT32 */
	8, 8,  /* PC_INT64, PC_UINT64 */
	8, 4   /* PC_DOUBLE, PC_FLOAT */
};


/**
* We need to hold a cached in-memory version of the format's
* XML structure for speed, and this is it.
*/
typedef struct
{
	char *name;
	char *description;
	uint32_t position;
	uint32_t size;
	uint32_t byteoffset;
	uint32_t interpretation;
	double scale;
	double offset;
	uint8_t active;
} PCDIMENSION;

typedef struct
{
	uint32_t pcid;        /* Unique ID for schema */
	uint32_t ndims;       /* How many dimensions does this schema have? */
	size_t size;          /* How wide (bytes) is a point with this schema? */
	PCDIMENSION **dims;   /* Array of dimension pointers */
	uint32_t srid;        /* Foreign key reference to SPATIAL_REF_SYS */
	uint32_t compression; /* Compression type applied to the data */
	hashtable *namehash;  /* Look-up from dimension name to pointer */
} PCSCHEMA;


/**
* Uncompressed structure for in-memory handling
* of points. A read-only PgSQL point can be wrapped in
* one of these by pointing the data element at the
* PgSQL memory and setting the capacity to 0
* to indicate it is read-only.
*/
typedef struct
{
	int8_t readonly;
	const PCSCHEMA *schema;
	uint8_t *data;
} PCPOINT;


/**
* Uncompressed Structure for in-memory handling
* of patches. A read-only PgSQL patch can be wrapped in
* one of these by pointing the data element at the
* PgSQL memory and setting the capacity to 0
* to indicate it is read-only.
*/
typedef struct
{
	int8_t readonly;
	size_t npoints; /* How many points we have */
	size_t maxpoints; /* How man points we can hold (or 0 for read-only) */
	const PCSCHEMA *schema;
	float xmin, xmax, ymin, ymax;
	uint8_t *data;
} PCPATCH;



/**
* Serialized point type for clouds. Variable length, because there can be
* an arbitrary number of dimensions. The pcid is a foreign key
* reference to the POINTCLOUD_SCHEMAS table, where
* the underlying structure of the data is described in XML,
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
// typedef struct
// {
// 	uint32_t size; /* PgSQL VARSIZE */
// 	uint32_t pcid;
// 	uint8_t data[1];
// } SERPOINT;

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
// typedef struct
// {
// 	uint32_t size; /* PgSQL VARSIZE */
// 	float xmin, xmax, ymin, ymax;
// 	uint32_t pcid;
// 	uint32_t npoints;
// 	uint8_t data[1];
// } SERPATCH;




/* Global function signatures for memory/logging handlers. */
typedef void* (*pc_allocator)(size_t size);
typedef void* (*pc_reallocator)(void *mem, size_t size);
typedef void  (*pc_deallocator)(void *mem);
typedef void  (*pc_message_handler)(const char *string, va_list ap);



/**********************************************************************
* MEMORY MANAGEMENT
*/

/** Allocate memory using the appropriate means (system/db) */
void* pcalloc(size_t size);
/** Reallocate memory using the appropriate means (system/db) */
void* pcrealloc(void* mem, size_t size);
/** Free memory using the appropriate means (system/db) */
void  pcfree(void* mem);
/** Emit an error message using the appropriate means (system/db) */
void  pcerror(const char *fmt, ...);
/** Emit an info message using the appropriate means (system/db) */
void  pcinfo(const char *fmt, ...);
/** Emit a warning message using the appropriate means (system/db) */
void  pcwarn(const char *fmt, ...);

/** Set custom memory allocators and messaging (used by PgSQL module) */
void pc_set_handlers(pc_allocator allocator, pc_reallocator reallocator,
                     pc_deallocator deallocator, pc_message_handler error_handler,
                     pc_message_handler info_handler, pc_message_handler warning_handler);

/** Set program to use system memory allocators and messaging */
void pc_install_default_handlers(void);



/**********************************************************************
* SCHEMAS
*/

/** Release the memory in a schema structure */
void pc_schema_free(PCSCHEMA *pcs);
/** Build a schema structure from the XML serialisation */
PCSCHEMA* pc_schema_from_xml(const char *xmlstr);
/** Print out JSON readable format of schema */
char* pc_schema_to_json(const PCSCHEMA *pcs);
/** Extract dimension information by position */
PCDIMENSION* pc_schema_get_dimension(const PCSCHEMA *s, uint32_t dim);
/** Extract dimension information by name */
PCDIMENSION* pc_schema_get_dimension_by_name(const PCSCHEMA *s, const char *name);



/**********************************************************************
* PCPOINT
*/

/** Create a new PCPOINT */
PCPOINT* pc_point_make(const PCSCHEMA *s);
/** Create a new PCPOINT on top of a data buffer */
PCPOINT* pc_point_make_from_data(const PCSCHEMA *s, uint8_t *data);
/** Frees the PTPOINT and data (if not readonly) does not free referenced schema */
void pc_point_free(PCPOINT *pt);
/** Casts named dimension value to double and scale/offset appropriately before returning */
double pc_point_get_double_by_name(const PCPOINT *pt, const char *name);
/** Casts dimension value to double and scale/offset appropriately before returning */
double pc_point_get_double_by_index(const PCPOINT *pt, uint32_t idx);
/** Scales/offsets double, casts to appropriate dimension type, and writes into point */
int pc_point_set_double_by_index(PCPOINT *pt, uint32_t idx, double val);
/** Scales/offsets double, casts to appropriate dimension type, and writes into point */
int pc_point_set_double_by_name(PCPOINT *pt, const char *name, double val);

/**********************************************************************
* PCPATCH
*/

/** Create a new PCPATCH */
PCPATCH* pc_patch_make(const PCSCHEMA *s);
/** Add a point to read/write PCPATCH */
int pc_patch_add_point(PCPATCH *c, const PCPOINT *p);



#endif /* _PC_API_H */