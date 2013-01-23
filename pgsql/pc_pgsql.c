
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
* WKB AND ENDIANESS UTILITIES
*/



/* Our static character->number map. Anything > 15 is invalid */
static uint8_t hex2char[256] = {
	/* not Hex characters */
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	/* 0-9 */
	0,1,2,3,4,5,6,7,8,9,20,20,20,20,20,20,
	/* A-F */
	20,10,11,12,13,14,15,20,20,20,20,20,20,20,20,20,
	/* not Hex characters */
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	/* a-f */
	20,10,11,12,13,14,15,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	/* not Hex characters (upper 128 characters) */
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20
};


uint8_t* 
bytes_from_hexbytes(const char *hexbuf, size_t hexsize)
{
	uint8_t *buf = NULL;
	register uint8_t h1, h2;
	int i;

	if( hexsize % 2 )
		pcerror("Invalid hex string, length (%d) has to be a multiple of two!", hexsize);

	buf = palloc(hexsize/2);

	if( ! buf )
		pcerror("Unable to allocate memory buffer.");

	for( i = 0; i < hexsize/2; i++ )
	{
		h1 = hex2char[(int)hexbuf[2*i]];
		h2 = hex2char[(int)hexbuf[2*i+1]];
		if( h1 > 15 )
			pcerror("Invalid hex character (%c) encountered", hexbuf[2*i]);
		if( h2 > 15 )
			pcerror("Invalid hex character (%c) encountered", hexbuf[2*i+1]);
		/* First character is high bits, second is low bits */
		buf[i] = ((h1 & 0x0F) << 4) | (h2 & 0x0F);
	}
	return buf;
}

char* 
hexbytes_from_bytes(const uint8_t *bytebuf, size_t bytesize)
{
	char *buf = palloc(2*bytesize + 1); /* 2 chars per byte + null terminator */
	int i;
	char *ptr = buf;
	
	for ( i = 0; i < bytesize; i++ )
	{
		int incr = snprintf(ptr, 3, "%02X", bytebuf[i]);
		if ( incr < 0 )
		{
			pcerror("write failure in hexbytes_from_bytes");
			return NULL;
		}
		ptr += incr;
	}
	
	return buf;
}

char
machine_endian(void)
{
	static int check_int = 1; /* dont modify this!!! */
	return *((char *) &check_int); /* 0 = big endian | xdr,
	                               * 1 = little endian | ndr
	                               */
}

static int32_t
int32_flip_endian(int32_t val)
{
	int i;
	uint8_t tmp;
	uint8_t b[4];
	memcpy(b, &val, 4);
	for ( i = 0; i < 2; i++ )
	{
		tmp = b[i];
		b[i] = b[3-i];
		b[3-i] = tmp;
	}
	memcpy(&val, b, 4);
	return val;
}

uint32_t
wkb_get_pcid(uint8_t *wkb)
{
	/* We expect the bytes to be in WKB format for PCPOINT/PCPATCH */
	/* byte 0:   endian */
	/* byte 1-4: pcid */
	/* ...data... */
	
	uint8_t wkb_endian = wkb[0];
	uint32_t pcid;
	memcpy(&pcid, wkb + 1, 4);
	if ( wkb_endian != machine_endian() )
	{
		pcid = int32_flip_endian(pcid);
	}
	return pcid;
}

PCPOINT *
pc_point_from_hexwkb(const char *hexwkb, size_t hexlen)
{
	PCPOINT *pt;
	uint8_t *wkb = bytes_from_hexbytes(hexwkb, hexlen);
	size_t wkblen = hexlen/2;
	pt = pc_point_from_wkb(wkb, wkblen);
	pfree(wkb);
	return pt;
}

PCPOINT *
pc_point_from_wkb(uint8_t *wkb, size_t wkblen)
{
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uchar[]:  data (interpret relative to pcid)
	*/
	const size_t hdrsz = 1+4; /* endian + pcid */
	uint8_t wkb_endian;
	uint32_t pcid;
	uint8_t *data;
	PCSCHEMA *schema;
	PCPOINT *pt;
	
	if ( ! wkblen )
	{
		elog(ERROR, "pc_point_from_wkb: zero length wkb");
	}
	
	wkb_endian = wkb[0];
	pcid = wkb_get_pcid(wkb);
	schema = pc_schema_get_by_id(pcid);
	
	if ( (wkblen-hdrsz) != schema->size )
	{
		elog(ERROR, "pc_point_from_wkb: wkb size inconsistent with schema size");
	}
	
	if ( wkb_endian != machine_endian() )
	{
		/* bytes_flip_endian creates a flipped copy */
		data = bytes_flip_endian(wkb+hdrsz, schema, 1);
	}
	else
	{
		data = palloc(schema->size);
		memcpy(data, wkb+hdrsz, wkblen-hdrsz);
	}

	pt = pc_point_from_data_rw(schema, data);
	return pt;
}

uint8_t *
wkb_from_point(const PCPOINT *pt, size_t *wkbsize)
{
	/*
	byte:     endianness (1 = NDR, 0 = XDR)
	uint32:   pcid (key to POINTCLOUD_SCHEMAS)
	uchar[]:  data (interpret relative to pcid)
	*/
	char endian = machine_endian();
	size_t size = 1 + 4 + pt->schema->size;
	uint8_t *wkb = palloc(size);
	wkb[0] = endian; /* Write endian flag */
	memcpy(wkb + 1, &(pt->schema->pcid), 4); /* Write PCID */
	memcpy(wkb + 5, pt->data, pt->schema->size); /* Write data */
	*wkbsize = size;
	return wkb;
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
	size_t serpt_size = sizeof(SERIALIZED_POINT) - 1 + pcpt->schema->size;
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
	size_t pgsize = VARSIZE(serpt) - 8; /* on-disk size - size:int32 - pcid:int32 == point data size */
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
	size_t serpch_size = sizeof(SERIALIZED_PATCH) - 1 + pcpch->schema->size * pcpch->npoints;
	SERIALIZED_PATCH *serpch = palloc(serpch_size);
	serpch->pcid = pcpch->schema->pcid;
	serpch->npoints = pcpch->npoints;
	serpch->xmin = pcpch->xmin;
	serpch->ymin = pcpch->ymin;
	serpch->xmax = pcpch->xmax;
	serpch->ymax = pcpch->ymax;
	memcpy(serpch->data, pcpch->data, pcpch->npoints * pcpch->schema->size);
	SET_VARSIZE(serpch, serpch_size);
	return serpch;
}

PCPATCH *
pc_patch_deserlialize(const SERIALIZED_PATCH *serpatch)
{
	PCPATCH *patch;
	PCSCHEMA *schema = pc_schema_get_by_id(serpatch->pcid);
	/* on-disk size - size:int32 - pcid:int32 - npoints:int32 - 4*minmax:double = patch data size */
	size_t pgsize = VARSIZE(serpatch) - 3*4 - 4*8; 
	if ( schema->size*serpatch->npoints != pgsize )
	{
		elog(ERROR, "schema size and disk size mismatch, repair the schema");
		return NULL;
	}
	
	/* Reference the external data */
	patch = pcalloc(sizeof(PCPATCH));
	patch->data = (uint8_t*)serpatch->data;
	
	/* Set up basic info */
	patch->schema = schema;
	patch->readonly = true;
	patch->npoints = serpatch->npoints;
	patch->xmin = serpatch->xmin;
	patch->ymin = serpatch->ymin;
	patch->xmax = serpatch->xmax;
	patch->ymax = serpatch->ymax;	

	return patch;
}

