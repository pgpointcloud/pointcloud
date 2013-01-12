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
enum INTERPRETATIONS
{
	PC_UNKNOWN = 0,
	PC_INT8,   PC_UINT8,
	PC_INT16,  PC_UINT16,
	PC_INT32,  PC_UINT32,
	PC_INT64,  PC_UINT64,
	PC_DOUBLE, PC_FLOAT
};

static char *INTERPRETATION_STRINGS[] =
{
	"unknown",
	"int8_t",  "uint8_t",
	"int16_t", "uint16_t",
	"int32_t", "uint32_t",
	"int64_t", "uint64_t",
	"double",  "float"
};




/**
* Point type for clouds. Variable length, because there can be
* an arbitrary number of dimensions. The pcid is a foreign key
* reference to the POINTCLOUD_SCHEMAS table, where
* the underlying structure of the data is described in XML,
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct
{
	uint32_t size; /* PgSQL VARSIZE */
	uint32_t pcid;
	uint8_t  data[1];
} PCPOINT;



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
	uint32_t size; /* PgSQL VARSIZE */
	float xmin, xmax, ymin, ymax;
	uint32_t npoints;
	uint32_t pcid;
	uint8_t  data[1];
} PCPATCH;


/**
* We need to hold a cached in-memory version of the formats
* XML structure for speed, and this is it.
*/
typedef struct
{
	char *name;
	char *description;
	uint32_t position;
	uint32_t size;
	uint32_t offset;
	uint32_t interpretation;
	double scale;
	uint8_t active;
} PCDIMENSION;

typedef struct
{
	uint32_t pcid;
	uint32_t ndims;
	PCDIMENSION **dims;
	uint32_t srid;
	uint32_t compression;
	struct hashtable *namehash;
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
	size_t size;
	int32_t readonly;
	PCSCHEMA *schema;
	uint8_t  *data;
} MEMPOINT;


/**
* Uncompressed Structure for in-memory handling
* of patches. A read-only PgSQL patch can be wrapped in
* one of these by pointing the data element at the
* PgSQL memory and setting the capacity to 0
* to indicate it is read-only.
*/
typedef struct
{
	size_t size;
	size_t capacity;
	int32_t readonly;
	float xmin, xmax, ymin, ymax;
	uint32_t npoints;
	PCSCHEMA *schema;
	uint8_t  *data;
} MEMPATCH;


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
PCSCHEMA* pc_schema_construct_from_xml(const char *xmlstr);
/** Print out JSON readable format of schema */
char* pc_schema_to_json(const PCSCHEMA *pcs);
/** Extract dimension information by position */
PCDIMENSION *pc_schema_get_dimension(const PCSCHEMA *s, uint32_t dim);
/** Extract dimension information by name */
PCDIMENSION *pc_schema_get_dimension_by_name(const PCSCHEMA *s, const char *name);



/**********************************************************************
* PCPOINT
*/

/** Create a new PCPOINT */
PCPOINT* pc_point_new(const PCSCHEMA *s);
/** Read the value of a dimension, as an int8 */
int8_t   pc_point_get_int8_t   (const PCPOINT *pt, uint32_t dim);
/** Read the value of a dimension, as an unsigned int8 */
uint8_t  pc_point_get_uint8_t  (const PCPOINT *pt, uint32_t dim);
/** Read the value of a dimension, as an int16 */
int16_t  pc_point_get_int16_t  (const PCPOINT *pt, uint32_t dim);
/** Read the value of a dimension, as an unsigned int16 */
uint16_t pc_point_get_uint16_t (const PCPOINT *pt, uint32_t dim);
/** Read the value of a dimension, as an int32_t */
int32_t  pc_point_get_int32_t  (const PCPOINT *pt, uint32_t dim);
/** Read the value of a dimension, as an unsigned int32_t */
uint32_t pc_point_get_uint32_t (const PCPOINT *pt, uint32_t dim);

#endif