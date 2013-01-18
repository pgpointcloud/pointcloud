
#include "pc_pgsql.h"
#include "executor/spi.h"

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
	schema = pc_schema_from_xml(xml);
	
	if ( ! schema )
	{
		elog(ERROR, "unable to parse XML representation of schema");
	}
	
	return schema;
}



