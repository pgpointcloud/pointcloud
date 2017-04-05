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
	PCPATCH *patch;
	float8 defaultvalue = 0.0;

	if ( mode == 0 )
	{
		bool reinterpret;
		if ( pc_schema_similar(schema, new_schema) )
		{
			// fast path!
			serpatch = palloc(serpa->size);
			if ( ! serpatch )
				PG_RETURN_NULL();
			memcpy(serpatch, serpa, serpa->size);
			serpatch->pcid = pcid;
			PG_RETURN_POINTER(serpatch);
		}
		reinterpret = PG_GETARG_BOOL(2);
		if ( ! reinterpret )
			pcwarn("incompatible schemas, and reinterpret is false");
	}
	else if ( mode == 1 )
	{
		defaultvalue = PG_GETARG_FLOAT8(2);
	}
	else
	{
		elog(ERROR, "unknown mode \"%d\"", mode);
		PG_RETURN_NULL();
	}

	patch = pc_patch_deserialize(serpa, schema);
	if ( ! patch )
		PG_RETURN_NULL();

	paout = pc_patch_set_schema(patch, new_schema, defaultvalue);

	if ( patch != paout )
		pc_patch_free(patch);

	if ( ! paout )
		PG_RETURN_NULL();

	serpatch = pc_patch_serialize(paout, NULL);
	pc_patch_free(paout);

	PG_RETURN_POINTER(serpatch);
}
