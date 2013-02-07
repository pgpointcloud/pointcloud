/***********************************************************************
* pc_pgsql.c
*
*  Utility functions to bind pc_api.h functions to PgSQL, including
*  memory management and serialization/deserializations.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_pgsql.h"
#include "executor/spi.h"
#include "access/hash.h"
#include "utils/hsearch.h"

PG_MODULE_MAGIC;

/**********************************************************************************
* POSTGRESQL MEMORY MANAGEMENT HOOKS 
*/

static void *
pgsql_alloc(size_t size)
{
	void * result;
	result = palloc(size);

	if ( ! result )
	{
        ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
             errmsg("Out of virtual memory")));
     }

	return result;
}

static void *
pgsql_realloc(void *mem, size_t size)
{
	void * result;
	result = repalloc(mem, size);
	if ( ! result )
	{
        ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
             errmsg("Out of virtual memory")));
     }
	return result;
}

static void
pgsql_free(void *ptr)
{
	pfree(ptr);
}

static void
pgsql_msg_handler(int sig, const char *fmt, va_list ap)
{
#define MSG_MAXLEN 1024
	char msg[MSG_MAXLEN] = {0};
	vsnprintf(msg, MSG_MAXLEN, fmt, ap);
	msg[MSG_MAXLEN-1] = '\0';
	ereport(sig, (errmsg_internal("%s", msg)));
}

static void
pgsql_error(const char *fmt, va_list ap)
{
	pgsql_msg_handler(ERROR, fmt, ap);
}

static void
pgsql_warn(const char *fmt, va_list ap)
{
	pgsql_msg_handler(WARNING, fmt, ap);
}

static void
pgsql_info(const char *fmt, va_list ap)
{
	pgsql_msg_handler(NOTICE, fmt, ap);
}

/**********************************************************************************
* POINTCLOUD START-UP/SHUT-DOWN CALLBACKS
*/

/**
* On module load we want to hook the message writing and memory allocation
* functions of libpc to the PostgreSQL ones. 
* TODO: also hook the libxml2 hooks into PostgreSQL.
*/
void _PG_init(void);
void
_PG_init(void)
{
	elog(LOG, "Pointcloud (%s) module loaded", POINTCLOUD_VERSION);
	pc_set_handlers(pgsql_alloc, pgsql_realloc,
	                pgsql_free, pgsql_error,
					pgsql_info, pgsql_warn);
}

/* Module unload callback */
void _PG_fini(void);
void
_PG_fini(void)
{
	elog(LOG, "Pointcloud (%s) module unloaded", POINTCLOUD_VERSION);
}

/* Mask pcid from bottom of typmod */
uint32 pcid_from_typmod(const int32 typmod)
{
    if ( typmod == -1 )
        return 0;
    else
        return (typmod & 0x0000FFFF);
}

/**********************************************************************************
* PCPOINT WKB Handling
*/

PCPOINT *
pc_point_from_hexwkb(const char *hexwkb, size_t hexlen, FunctionCallInfoData *fcinfo)
{
	PCPOINT *pt;
	PCSCHEMA *schema;
	uint32 pcid;
	uint8 *wkb = bytes_from_hexbytes(hexwkb, hexlen);
	size_t wkblen = hexlen/2;
	pcid = wkb_get_pcid(wkb);	
	schema = pc_schema_from_pcid(pcid, fcinfo);
	pt = pc_point_from_wkb(schema, wkb, wkblen);	
	pfree(wkb);
	return pt;
}

char *
pc_point_to_hexwkb(const PCPOINT *pt)
{
	uint8 *wkb;
	size_t wkb_size;
	char *hexwkb;
	
	wkb = pc_point_to_wkb(pt, &wkb_size);
	hexwkb = hexbytes_from_bytes(wkb, wkb_size);
	pfree(wkb);
	return hexwkb;
}


/**********************************************************************************
* PCPATCH WKB Handling
*/

PCPATCH *
pc_patch_from_hexwkb(const char *hexwkb, size_t hexlen, FunctionCallInfoData *fcinfo)
{
	PCPATCH *patch;
	PCSCHEMA *schema;
	uint32 pcid;
	uint8 *wkb = bytes_from_hexbytes(hexwkb, hexlen);
	size_t wkblen = hexlen/2;
	pcid = wkb_get_pcid(wkb);	
	schema = pc_schema_from_pcid(pcid, fcinfo);
	patch = pc_patch_from_wkb(schema, wkb, wkblen);	
	pfree(wkb);
	return patch;
}

char *
pc_patch_to_hexwkb(const PCPATCH *patch)
{
	uint8 *wkb;
	size_t wkb_size;
	char *hexwkb;
	
	wkb = pc_patch_to_wkb(patch, &wkb_size);
	hexwkb = hexbytes_from_bytes(wkb, wkb_size);
	pfree(wkb);
	return hexwkb;
}


/**********************************************************************************
* PCID <=> PCSCHEMA translation via POINTCLOUD_FORMATS
*/

uint32 pcid_from_datum(Datum d)
{
    SERIALIZED_POINT *serpart;
    if ( ! d )
        return 0;
    /* Serializations are int32_t <size> uint32_t <pcid> == 8 bytes */
    /* Cast to SERIALIZED_POINT for convenience, SERIALIZED_PATCH shares same header */
    serpart = (SERIALIZED_POINT*)PG_DETOAST_DATUM_SLICE(d, 0, 8);
    return serpart->pcid;
}

PCSCHEMA *
pc_schema_from_pcid_uncached(uint32 pcid)
{
	char sql[256];
	char *xml, *xml_spi, *srid_spi;
	int err, srid;
	size_t size;
	PCSCHEMA *schema;

	if (SPI_OK_CONNECT != SPI_connect ())
	{
		SPI_finish();
		elog(ERROR, "pc_schema_from_pcid: could not connect to SPI manager");
		return NULL;
	}

	sprintf(sql, "select %s, %s from %s where pcid = %d", 
	              POINTCLOUD_FORMATS_XML, POINTCLOUD_FORMATS_SRID, POINTCLOUD_FORMATS, pcid);
	err = SPI_exec(sql, 1);

	if ( err < 0 )
	{
		SPI_finish();
		elog(ERROR, "pc_schema_from_pcid: error (%d) executing query: %s", err, sql);
		return NULL;
	} 

	/* No entry in POINTCLOUD_FORMATS */
	if (SPI_processed <= 0)
	{
		SPI_finish();
		elog(ERROR, "no entry in \"%s\" for pcid = %d", POINTCLOUD_FORMATS, pcid);
		return NULL;
	}

	/* Result  */
	xml_spi = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);
	srid_spi = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 2);	

	/* NULL result */
	if ( ! ( xml_spi && srid_spi ) )
	{
		SPI_finish();
		elog(ERROR, "unable to read row from \"%s\" for pcid = %d", POINTCLOUD_FORMATS, pcid);
		return NULL;
	}

	/* Copy result to upper executor context */
	size = strlen(xml_spi) + 1;
	xml = SPI_palloc(size);
	memcpy(xml, xml_spi, size);

	/* Parse the SRID string into the function stack */
	srid = atoi(srid_spi);

	/* Disconnect from SPI, losing all our SPI-allocated memory now... */
	SPI_finish();

	/* Build the schema object */
	err = pc_schema_from_xml(xml, &schema);
	
	if ( ! err )
	{
        ereport(ERROR,
            (errcode(ERRCODE_NOT_AN_XML_DOCUMENT),
             errmsg("unable to parse XML for pcid = %d in \"%s\"", pcid, POINTCLOUD_FORMATS)));
	}
	
	schema->pcid = pcid;
	schema->srid = srid;
	
	return schema;
}


typedef struct {
    int type;
    char data[1];
} GenericCache;

/**
* Hold the schema references in a list.
* We'll just search them linearly, because
* usually we'll have only one per statement
*/
#define SchemaCacheSize 16

typedef struct {
    int type;
    int next_slot;
    int pcids[SchemaCacheSize];
    PCSCHEMA* schemas[SchemaCacheSize];
} SchemaCache;


/**
* PostGIS uses this kind of structure for its
* cache objects, and we expect to be used in
* concert with PostGIS, so here we not only ape
* the container, but avoid the first 10 slots,
* so as to miss any existing cached PostGIS objects.
*/ 
typedef struct {
    GenericCache *entry[16];
} GenericCacheCollection;

#define PC_SCHEMA_CACHE 10
#define PC_STATS_CACHE  11
 
/**
* Get the generic collection off the statement, allocate a 
* new one if we don't have one already.
*/ 
static GenericCacheCollection * 
GetGenericCacheCollection(FunctionCallInfoData *fcinfo)
{
    GenericCacheCollection *cache = fcinfo->flinfo->fn_extra;

    if ( ! cache ) 
    {
        cache = MemoryContextAlloc(fcinfo->flinfo->fn_mcxt, sizeof(GenericCacheCollection));
        memset(cache, 0, sizeof(GenericCacheCollection));
        fcinfo->flinfo->fn_extra = cache;
    }
    return cache;
}

/**
* Get the Proj4 entry from the generic cache if one exists.
* If it doesn't exist, make a new empty one and return it.
*/
static SchemaCache *
GetSchemaCache(FunctionCallInfoData* fcinfo)
{
        GenericCacheCollection *generic_cache = GetGenericCacheCollection(fcinfo);
        SchemaCache* cache = (SchemaCache*)(generic_cache->entry[PC_SCHEMA_CACHE]);
        
        if ( ! cache )
        {
                /* Allocate in the upper context */
                cache = MemoryContextAlloc(fcinfo->flinfo->fn_mcxt, sizeof(SchemaCache));
                memset(cache, 0, sizeof(SchemaCache));
                cache->type = PC_SCHEMA_CACHE;
        }
        
        generic_cache->entry[PC_SCHEMA_CACHE] = (GenericCache*)cache;
        return cache;
}


PCSCHEMA *
pc_schema_from_pcid(uint32 pcid, FunctionCallInfoData *fcinfo)
{
    SchemaCache *schema_cache = GetSchemaCache(fcinfo);
    int i;
    PCSCHEMA *schema;
    MemoryContext oldcontext;
    
    /* Unable to find/make a schema cache? Odd. */
    if ( ! schema_cache )
    {
        ereport(ERROR,
            (errcode(ERRCODE_INTERNAL_ERROR),
             errmsg("unable to create/load statement level schema cache")));
    }
    
    /* Find our PCID if it's in there (usually it will be first) */
    for ( i = 0; i < SchemaCacheSize; i++ )
    {
        if ( schema_cache->pcids[i] == pcid )
        {
            return schema_cache->schemas[i];
        }
    }
    
    /* Not in there, load one the old-fashioned way. */
    oldcontext = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
    schema = pc_schema_from_pcid_uncached(pcid);
    MemoryContextSwitchTo(oldcontext);
    
    /* Failed to load the XML? Odd. */
    if ( ! schema )
    {
        ereport(ERROR,
            (errcode(ERRCODE_INTERNAL_ERROR),
             errmsg("unable to load schema for pcid %u", pcid)));
    }

    /* Save the XML in the next unused slot */
    schema_cache->schemas[schema_cache->next_slot] = schema;
    schema_cache->pcids[schema_cache->next_slot] = pcid;
    schema_cache->next_slot = (schema_cache->next_slot + 1) % SchemaCacheSize;
    return schema;
}



/**********************************************************************************
* SERIALIZATION/DESERIALIZATION UTILITIES
*/

SERIALIZED_POINT * 
pc_point_serialize(const PCPOINT *pcpt)
{
	size_t serpt_size = sizeof(SERIALIZED_POINT) - 1 + pcpt->schema->size;
	SERIALIZED_POINT *serpt = palloc(serpt_size);
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
		elog(ERROR, "schema size and disk size mismatch, repair the schema");
		return NULL;
	}
	pcpt = pc_point_from_data(schema, serpt->data);
	return pcpt;
}


SERIALIZED_PATCH * 
pc_patch_serialize(const PCPATCH *pcpch)
{
	size_t serpch_size;
	PCPATCH *patch;
	SERIALIZED_PATCH *serpch;
	
	/* Compress uncompressed patches before saving */
	patch = pc_patch_compress(pcpch);
	
	/* Allocate: size(int4) + pcid(int4) + npoints(int4) + box(4*float8) + data(?) */
	serpch_size = sizeof(SERIALIZED_PATCH) - 1 + patch->datasize; 
	serpch = palloc(serpch_size);
	
	/* Copy */
	serpch->pcid = patch->schema->pcid;
	serpch->npoints = patch->npoints;
	serpch->xmin = patch->xmin;
	serpch->ymin = patch->ymin;
	serpch->xmax = patch->xmax;
	serpch->ymax = patch->ymax;
	memcpy(serpch->data, patch->data, patch->datasize);
	SET_VARSIZE(serpch, serpch_size);
	return serpch;
}

PCPATCH *
pc_patch_deserialize(const SERIALIZED_PATCH *serpatch, const PCSCHEMA *schema)
{
	PCPATCH *patch;
	
	/* Reference the external data */
	patch = pcalloc(sizeof(PCPATCH));
	patch->data = (uint8*)serpatch->data;
	patch->datasize = VARSIZE(serpatch) - sizeof(SERIALIZED_PATCH) + 1;

	/* Set up basic info */
	patch->schema = schema;
	patch->readonly = true;
	patch->compressed = true;
	patch->npoints = serpatch->npoints;
	patch->maxpoints = 0;
	patch->xmin = serpatch->xmin;
	patch->ymin = serpatch->ymin;
	patch->xmax = serpatch->xmax;
	patch->ymax = serpatch->ymax;

	return patch;
}

