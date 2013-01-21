
#include "pc_pgsql.h"
#include "executor/spi.h"

PG_MODULE_MAGIC;


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

/* Module loading callback */
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


/**
* TODO: Back this routine with a statement level memory cache.
*/
PCSCHEMA *
pc_get_schema_by_id(uint32_t pcid)
{
	char sql[256];
	char *xml, *xml_spi;
	int err;
	size_t size;
	PCSCHEMA *schema;

	if (SPI_OK_CONNECT != SPI_connect ())
	{
		elog(NOTICE, "pc_get_schema_by_id: could not connect to SPI manager");
		SPI_finish();
		return NULL;
	}

	sprintf(sql, "select %s from %s where pcid = %d", POINTCLOUD_FORMATS, POINTCLOUD_FORMATS_XML, pcid);
	err = SPI_exec(sql, 1);

	if ( err < 0 )
	{
		elog(NOTICE, "pc_get_schema_by_id: error (%d) executing query: %s", err, sql);
		SPI_finish();
		return NULL;
	} 

	/* No entry in POINTCLOUD_FORMATS */
	if (SPI_processed <= 0)
	{
		SPI_finish();
		return NULL;
	}

	/* Result  */
	xml_spi = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);

	/* NULL result */
	if ( ! xml_spi )
	{
		SPI_finish();
		return NULL;
	}

	/* Copy result to upper executor context */
	size = strlen(xml_spi) + 1;
	xml = SPI_palloc(size);
	memcpy(xml, xml_spi, size);

	/* Disconnect from SPI */
	SPI_finish();

	/* Build the schema object */
	err = pc_schema_from_xml(xml, &schema);
	
	if ( ! err )
	{
		elog(ERROR, "unable to parse XML representation of schema");
		return NULL;
	}
	
	return schema;
}



