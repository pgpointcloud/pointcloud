
#include "pc_pgsql.h"
#include "executor/spi.h"

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
		ereport(ERROR, (errmsg_internal("Out of virtual memory")));
		return NULL;
	}
	return result;
}

static void *
pgsql_realloc(void *mem, size_t size)
{
	void * result;
	result = repalloc(mem, size);
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


/**********************************************************************************
* PCPOINT WKB Handling
*/

PCPOINT *
pc_point_from_hexwkb(const char *hexwkb, size_t hexlen)
{
	PCPOINT *pt;
	PCSCHEMA *schema;
	uint32_t pcid;
	uint8_t *wkb = bytes_from_hexbytes(hexwkb, hexlen);
	size_t wkblen = hexlen/2;
	pcid = wkb_get_pcid(wkb);	
	schema = pc_schema_get_by_id(pcid);
	pt = pc_point_from_wkb(schema, wkb, wkblen);	
	pfree(wkb);
	return pt;
}

char *
pc_point_to_hexwkb(const PCPOINT *pt)
{
	uint8_t *wkb;
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
pc_patch_from_hexwkb(const char *hexwkb, size_t hexlen)
{
	PCPATCH *patch;
	PCSCHEMA *schema;
	uint32_t pcid;
	uint8_t *wkb = bytes_from_hexbytes(hexwkb, hexlen);
	size_t wkblen = hexlen/2;
	pcid = wkb_get_pcid(wkb);	
	schema = pc_schema_get_by_id(pcid);
	patch = pc_patch_from_wkb(schema, wkb, wkblen);	
	pfree(wkb);
	return patch;
}

char *
pc_patch_to_hexwkb(const PCPATCH *patch)
{
	uint8_t *wkb;
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

/**
* TODO: Back this routine with a statement level memory cache.
*/
PCSCHEMA *
pc_schema_get_by_id(uint32_t pcid)
{
	char sql[256];
	char *xml, *xml_spi, *srid_spi;
	int err, srid;
	size_t size;
	PCSCHEMA *schema;

	if (SPI_OK_CONNECT != SPI_connect ())
	{
		SPI_finish();
		elog(ERROR, "pc_schema_get_by_id: could not connect to SPI manager");
		return NULL;
	}

	sprintf(sql, "select %s, %s from %s where pcid = %d", 
	              POINTCLOUD_FORMATS_XML, POINTCLOUD_FORMATS_SRID, POINTCLOUD_FORMATS, pcid);
	err = SPI_exec(sql, 1);

	if ( err < 0 )
	{
		SPI_finish();
		elog(ERROR, "pc_schema_get_by_id: error (%d) executing query: %s", err, sql);
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
		elog(ERROR, "unable to parse XML of pcid = %d in \"%s\"", pcid, POINTCLOUD_FORMATS);
		return NULL;
	}
	
	schema->pcid = pcid;
	schema->srid = srid;
	
	return schema;
}


/**********************************************************************************
* SERIALIZATION/DESERIALIZATION UTILITIES
*/

SERIALIZED_POINT * 
pc_point_serialize(const PCPOINT *pcpt)
{
	size_t serpt_size = 4 + 4 + pcpt->schema->size;
	SERIALIZED_POINT *serpt = palloc(serpt_size);
	serpt->pcid = pcpt->schema->pcid;
	memcpy(serpt->data, pcpt->data, pcpt->schema->size);
	SET_VARSIZE(serpt, serpt_size);
	return serpt;
}

PCPOINT * 
pc_point_deserialize(const SERIALIZED_POINT *serpt)
{
	PCPOINT *pcpt;
	PCSCHEMA *schema = pc_schema_get_by_id(serpt->pcid);
	size_t pgsize = VARSIZE(serpt) - 4 - 4; /* on-disk size - size:int32 - pcid:int32 == point data size */
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
	serpch_size = 4+4+4+4*8+patch->datasize; 
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
pc_patch_deserialize(const SERIALIZED_PATCH *serpatch)
{
	PCPATCH *patch;
	PCSCHEMA *schema = pc_schema_get_by_id(serpatch->pcid);
	
	/* Reference the external data */
	patch = pcalloc(sizeof(PCPATCH));
	patch->data = (uint8_t*)serpatch->data;

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

