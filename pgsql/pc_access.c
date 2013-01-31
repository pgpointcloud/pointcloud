/***********************************************************************
* pc_access.c
*
*  Accessor functions for points and patches in PgSQL.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_pgsql.h"      /* Common PgSQL support for our type */
#include "utils/numeric.h"

/* Other SQL functions */
Datum PC_Get(PG_FUNCTION_ARGS);



/**
* Read a named dimension from a PCPOINT
* PC_Get(point pcpoint, dimname text) returns Numeric
*/
PG_FUNCTION_INFO_V1(PC_Get);
Datum PC_Get(PG_FUNCTION_ARGS)
{
	SERIALIZED_POINT *serpt = PG_GETARG_SERPOINT_P(0);
	text *dim_name = PG_GETARG_TEXT_P(1);
	char *dim_str;
	float8 double_result;

	PCPOINT *pt = pc_point_deserialize(serpt);
	if ( ! pt ) 
		PG_RETURN_NULL();	
	
	dim_str = text_to_cstring(dim_name);
	if ( ! pc_schema_get_dimension_by_name(pt->schema, dim_str) )
	{
		pc_point_free(pt);
		elog(ERROR, "dimension \"%s\" does not exist in schema", dim_str);
	}
	
	double_result = pc_point_get_double_by_name(pt, dim_str);
	pfree(dim_str);
	pc_point_free(pt);
	PG_RETURN_DATUM(DirectFunctionCall1(float8_numeric, Float8GetDatum(double_result)));
}

