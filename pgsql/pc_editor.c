/***********************************************************************
* pc_editor.c
*
*  Editor functions for points and patches in PgSQL.
*
*  Copyright (c) 2017 Oslandia
*
***********************************************************************/

#include "pc_pgsql.h"	   /* Common PgSQL support for our type */

Datum pcpatch_setpcid(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pcpatch_setpcid);
Datum pcpatch_setpcid(PG_FUNCTION_ARGS)
{
	PCPATCH *paout = NULL;
	SERIALIZED_PATCH *serpatch;
	SERIALIZED_PATCH *serpa = PG_GETARG_SERPATCH_P(0);
	int32 pcid = PG_GETARG_INT32(1);
	bool reinterpret = PG_GETARG_BOOL(2);
	float8 defaultvalue = PG_GETARG_FLOAT8(3);
	PCSCHEMA *schema = pc_schema_from_pcid(serpa->pcid, fcinfo);
	PCSCHEMA *new_schema = pc_schema_from_pcid(pcid, fcinfo);

	if ( pc_schema_equivalent(schema, new_schema) )
	{
		if ( schema->compression == new_schema->compression )
		{
			// no need to deserialize the patch
			serpatch = palloc(serpa->size);
			if ( ! serpatch )
				PG_RETURN_NULL();
			memcpy(serpatch, serpa, serpa->size);
			serpatch->pcid = pcid;
			PG_RETURN_POINTER(serpatch);
		}
		else
		{
			paout = pc_patch_deserialize(serpa, schema);
			if ( ! paout )
				PG_RETURN_NULL();
			paout->schema = new_schema;
		}
	} else {
		PCPATCH *patch;

		if ( ! reinterpret )
		{
			pcerror("incompatible schemas, and reinterpret is false");
			PG_RETURN_NULL();
		}

		patch = pc_patch_deserialize(serpa, schema);
		if ( ! patch )
			PG_RETURN_NULL();

		paout = pc_patch_set_pcid(patch, new_schema, defaultvalue);

		if ( patch != paout )
			pc_patch_free(patch);
	}

	if ( ! paout )
		PG_RETURN_NULL();

	serpatch = pc_patch_serialize(paout, NULL);
	pc_patch_free(paout);

	PG_RETURN_POINTER(serpatch);
}
