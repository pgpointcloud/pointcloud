
#include "pc_pgsql.h"      /* Common PgSQL support for our type */

/* In/out functions */
Datum pcpoint_in(PG_FUNCTION_ARGS);
Datum pcpoint_out(PG_FUNCTION_ARGS);

/* Other SQL functions */
Datum PC_SchemaIsValid(PG_FUNCTION_ARGS);
Datum PC_SchemaGetNDims(PG_FUNCTION_ARGS);
Datum PC_MakePointFromArray(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pcpoint_in);
Datum pcpoint_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	/* Datum geog_oid = PG_GETARG_OID(1); Not needed. */
	int32 pc_typmod = -1;
	PCPOINT *pt;
	SERIALIZED_POINT *serpt;
	
	if ( (PG_NARGS()>2) && (!PG_ARGISNULL(2)) ) 
	{
		pc_typmod = PG_GETARG_INT32(2);
	}

	/* Empty string. */
	if ( str[0] == '\0' )
	{
		ereport(ERROR,(errmsg("pcpoint parse error - empty string")));
	}

	/* Binary or text form? Let's find out. */
	if ( str[0] == '0' )
	{		
		/* Hex-encoded binary */
		pt = pc_point_from_hexwkb(str, strlen(str));
		serpt = pc_point_serialize(pt);
		pc_point_free(pt);
	}
	else
	{
		ereport(ERROR,(errmsg("parse error - support for text format not yet implemented")));
	}

	PG_RETURN_POINTER(serpt);
}

PG_FUNCTION_INFO_V1(pcpoint_out);
Datum pcpoint_out(PG_FUNCTION_ARGS)
{
	PCPOINT *pcpt = NULL;
	SERIALIZED_POINT *serpt = NULL;
	uint8_t *wkb = NULL;
	size_t wkb_size = 0;
	char *hexwkb = NULL;

	serpt = (SERIALIZED_POINT*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pcpt = pc_point_deserialize(serpt);
	wkb = wkb_from_point(pcpt, &wkb_size);
	hexwkb = hexbytes_from_bytes(wkb, wkb_size);
	PG_RETURN_CSTRING(hexwkb);
}


PG_FUNCTION_INFO_V1(PC_SchemaIsValid);
Datum PC_SchemaIsValid(PG_FUNCTION_ARGS)
{
	bool valid;
	text *xml = PG_GETARG_TEXT_P(0);
	char *xmlstr = text_to_cstring(xml);
	PCSCHEMA *schema;
	int err = pc_schema_from_xml(xmlstr, &schema);
	pfree(xmlstr);
	
	if ( ! err )
	{
		PG_RETURN_BOOL(FALSE);
	}

	valid = pc_schema_is_valid(schema);
	PG_RETURN_BOOL(valid);
}

PG_FUNCTION_INFO_V1(PC_SchemaGetNDims);
Datum PC_SchemaGetNDims(PG_FUNCTION_ARGS)
{
	int ndims;
	uint32 pcid = PG_GETARG_INT32(0);
	PCSCHEMA *schema = pc_schema_get_by_id(pcid);

	if ( ! schema )
		elog(ERROR, "unable to load schema for pcid = %d", pcid);

	ndims = schema->ndims;
	pc_schema_free(schema);
	PG_RETURN_INT32(ndims);
}

/**
* PC_MakePointFromArray(integer pcid, float8[] returns PcPoint
*/
PG_FUNCTION_INFO_V1(PC_MakePointFromArray);
Datum PC_MakePointFromArray(PG_FUNCTION_ARGS)
{
	uint32 pcid = PG_GETARG_INT32(0);
	ArrayType *arrptr = PG_GETARG_ARRAYTYPE_P(1);
	int nelems;
	int i;
	float8 *vals;
	PCPOINT *pt;
	PCSCHEMA *schema = pc_schema_get_by_id(pcid);
	SERIALIZED_POINT *serpt;

	if ( ! schema )
		elog(ERROR, "unable to load schema for pcid = %d", pcid);

	if ( ARR_ELEMTYPE(arrptr) != FLOAT8OID )
		elog(ERROR, "array must be of float8[]");
		
	if ( ARR_NDIM(arrptr) != 1 )
		elog(ERROR, "float8[] must have only one dimension");

	if ( ARR_HASNULL(arrptr) )
		elog(ERROR, "float8[] must not have null elements");
	
	nelems = ARR_DIMS(arrptr)[0];
	if ( nelems != schema->ndims || ARR_LBOUND(arrptr)[0] > 1 )
		elog(ERROR, "array dimenensions do not match schema dimensions of pcid = %d", pcid);
	
	pt = pc_point_make(schema);
	vals = (float8*) ARR_DATA_PTR(arrptr);

	for ( i = 0; i < nelems; i++ )
	{
		float8 val = vals[i];
		int err = pc_point_set_double_by_index(pt, i, val);
		if ( ! err )
			elog(ERROR, "failed to set value into point");		
	}
	
	serpt = pc_point_serialize(pt);
	pc_point_free(pt);
	PG_RETURN_POINTER(serpt);
}