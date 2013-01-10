/***********************************************************************
* pc_core.h
*
*        Structures and function signatures for point clouds
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>

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
    PC_INT8,   PC_UINT8,
    PC_INT16,  PC_UINT16,
    PC_INT32,  PC_UINT32,
    PC_INT64,  PC_UINT64,
    PC_DOUBLE, PC_FLOAT, PC_UNKNOWN
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
	uint32 size; /* PgSQL VARSIZE */
	uint8  endian;
	uint16 pcid;
	uint8  data[1];
} PCPOINT;


/**
* Generic patch type (collection of points) for clouds.
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
	uint32 size; /* PgSQL VARSIZE */
	uint8  endian;
	uint8  spacer; /* Unused */
	uint16 pcid;
	float xmin, xmax, ymin, ymax;
	uint32 npoints;
	uint8  data[1];
} PCPATCH;


/**
* We need to hold a cached in-memory version of the formats
* XML structure for speed, and this is it.
*/
typedef struct
{
	uint32 size;
	char *description;
	char *name;
	uint32 interpretation;
	double scale;
	uint8 active;
} PCDIMENSION;

typedef struct
{
	uint32 pcid;
	uint32 ndims;
	PCDIMENSION *dims;
} PCSCHEMA;

/* Global function signatures for memory/logging handlers. */
typedef void* (*pc_allocator)(size_t size);
typedef void* (*pc_reallocator)(void *mem, size_t size);
typedef void  (*pc_deallocator)(void *mem);
typedef void  (*pc_message_handler)(const char *string, va_list ap);

/**********************************************************************
* FUNCTION PROTOTYPES
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

/** Release the memory in a schema structure */
void pc_schema_free(PCSCHEMA *pcf);
/** Build a schema structure from the XML serialisation */
PCSCHEMA* pc_schema_construct(const char *xmlstr);

/** Read the value of a dimension, as an int8 */
int8   pc_point_get_int8   (const PCPOINT *pt, uint32 dim);
/** Read the value of a dimension, as an unsigned int8 */
uint8  pc_point_get_uint8  (const PCPOINT *pt, uint32 dim);
/** Read the value of a dimension, as an int16 */
int16  pc_point_get_int16  (const PCPOINT *pt, uint32 dim);
/** Read the value of a dimension, as an unsigned int16 */
uint16 pc_point_get_uint16 (const PCPOINT *pt, uint32 dim);
/** Read the value of a dimension, as an int32 */
int32  pc_point_get_int32  (const PCPOINT *pt, uint32 dim);
/** Read the value of a dimension, as an unsigned int32 */
uint32 pc_point_get_uint32 (const PCPOINT *pt, uint32 dim);

