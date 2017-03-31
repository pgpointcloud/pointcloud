/***********************************************************************
* pc_editor.c
*
*  Editor functions for points and patches in PgSQL.
*
*  Copyright (c) 2017 Oslandia
*
***********************************************************************/

#include "pc_pgsql.h"	   /* Common PgSQL support for our type */

Datum pcpatch_setschema(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pcpatch_setschema);
Datum pcpatch_setschema(PG_FUNCTION_ARGS)
{
	PCPATCH *paout = NULL;
	SERIALIZED_PATCH *serpatch;
	SERIALIZED_PATCH *serpa = PG_GETARG_SERPATCH_P(0);
	int32 pcid = PG_GETARG_INT32(1);
	int32 mode = PG_GETARG_INT32(3);
	PCSCHEMA *schema = pc_schema_from_pcid(serpa->pcid, fcinfo);
	PCSCHEMA *new_schema = pc_schema_from_pcid(pcid, fcinfo);
	PCPATCH *patch = pc_patch_deserialize(serpa, schema);
	bool reinterpret;
	float8 defaultvalue;

	if ( ! patch )
		PG_RETURN_NULL();

	switch ( mode )
	{
	case 0:
		reinterpret = PG_GETARG_BOOL(2);
		paout = pc_patch_set_schema(patch, new_schema, reinterpret, 0.0);
		break;
	case 1:
		defaultvalue = PG_GETARG_FLOAT8(2);
		paout = pc_patch_set_schema(patch, new_schema, 1, defaultvalue);
		break;
	default:
		elog(ERROR, "unknown mode \"%d\"", mode);
		PG_RETURN_NULL();
	}

	if ( patch != paout )
		pc_patch_free(patch);

	if ( !paout )
		PG_RETURN_NULL();

	serpatch = pc_patch_serialize(paout, NULL);
	pc_patch_free(paout);

	PG_RETURN_POINTER(serpatch);
}
