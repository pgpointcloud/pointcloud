/***********************************************************************
* pc_api_internal.h
*
*  Signatures we need to share within the library, but not for 
*  use outside it.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#ifndef _PC_API_INTERNAL_H
#define _PC_API_INTERNAL_H

#include "pc_api.h"

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

#define PCPATCH_DEFAULT_MAXPOINTS 64

/**
* Interpretation types for our dimension descriptions
*/
#define NUM_INTERPRETATIONS 11

enum INTERPRETATIONS
{
	PC_UNKNOWN = 0,
	PC_INT8   = 1,  PC_UINT8  = 2,
	PC_INT16  = 3,  PC_UINT16 = 4,
	PC_INT32  = 5,  PC_UINT32 = 6,
	PC_INT64  = 7,  PC_UINT64 = 8,
	PC_DOUBLE = 9,  PC_FLOAT  = 10
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

enum DIMCOMPRESSIONS {
    PC_DIM_NONE = 0,
    PC_DIM_RLE = 1,
    PC_DIM_SIGBITS = 2,
    PC_DIM_ZLIB = 3
};

typedef struct
{
	uint32_t npoints;
	const PCSCHEMA *schema;
    uint32_t *compressions;
    uint8_t **data;
} PCDIMLIST;

typedef struct
{
    uint32_t total_runs;
    uint32_t total_commonbits;
    uint32_t recommended_compression;
} PCDIMSTAT;

typedef struct
{
    int32_t ndims;
    uint32_t total_points;
    uint32_t total_patches;
    PCDIMSTAT *stats;
} PCDIMSTATS;



/** What is the endianness of this system? */
char machine_endian(void);

/** Force a byte array into the machine endianness */
uint8_t* uncompressed_bytes_flip_endian(const uint8_t *bytebuf, const PCSCHEMA *schema, uint32_t npoints);

/** Read interpretation type from buffer and cast to double */
double pc_double_from_ptr(const uint8_t *ptr, uint32_t interpretation);

/** Write value to buffer in the interpretation type */
int pc_double_to_ptr(uint8_t *ptr, uint32_t interpretation, double val);

/** Return number of bytes in a given interpretation */
size_t pc_interpretation_size(uint32_t interp);

/** True if there is a dimension of that name */
int pc_schema_has_name(const PCSCHEMA *s, const char *name);

/** Copy a string within the global memory management context */
char* pcstrdup(const char *str);

/** How many runs are there in a value array? */
uint32_t pc_bytes_run_count(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems);

/** Convert value bytes to RLE bytes */
uint8_t* pc_bytes_run_length_encode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems, size_t *bytes_rle_size);

/** Convert RLE bytes to value bytes */
uint8_t* pc_bytes_run_length_decode(const uint8_t *bytes_rle, size_t bytes_rle_size, uint32_t interpretation, uint32_t *bytes_nelems);

/** How many bits are shared by all elements of this array? */
uint32_t pc_sigbits_count(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems);
uint8_t  pc_sigbits_count_8 (const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits);
uint16_t pc_sigbits_count_16(const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits);
uint32_t pc_sigbits_count_32(const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits);
uint64_t pc_sigbits_count_64(const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits);
uint8_t* pc_bytes_sigbits_encode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems, size_t *bytes_sigbits_size);
uint8_t* pc_bytes_sigbits_decode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems);

uint8_t* pc_bytes_zlib_decode(const uint8_t *bytes, uint32_t interpretation);
uint8_t* pc_bytes_zlib_encode(const uint8_t *bytes, uint32_t interpretation,  uint32_t nelems);

PCDIMLIST* pc_dimlist_from_pointlist(const PCPOINTLIST *pl);
PCPOINTLIST* pc_pointlist_from_dimlist(PCDIMLIST *pdl);

int pc_dimlist_encode(PCDIMLIST *pdl);
int pc_dimlist_decode(PCDIMLIST *pdl);

PCDIMSTATS* pc_dimstats_make(const PCSCHEMA *schema);
int pc_dimstats_update(PCDIMSTATS *pds, const PCDIMLIST *pdl);
void pc_dimstats_free(PCDIMSTATS *pds);

#endif /* _PC_API_INTERNAL_H */