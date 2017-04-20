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


static bool
pcpatch_schema_same_dimensions(const PCSCHEMA *s1, const PCSCHEMA *s2)
{
	size_t i;

	if ( s1->ndims != s2->ndims )
		return false;

	for ( i = 0; i < s1->ndims; i++ )
	{
		PCDIMENSION *s1dim = s1->dims[i];
		PCDIMENSION *s2dim = s2->dims[i];

		if ( strcasecmp(s1dim->name, s2dim->name) != 0 )
			return false;

		if ( s1dim->interpretation != s2dim->interpretation )
			return false;
	}

	return true;
}


PG_FUNCTION_INFO_V1(pcpatch_setpcid);
Datum pcpatch_setpcid(PG_FUNCTION_ARGS)
{
	PCPATCH *paout = NULL;
	SERIALIZED_PATCH *serpatch;
	SERIALIZED_PATCH *serpa = PG_GETARG_SERPATCH_P(0);
	int32 pcid = PG_GETARG_INT32(1);
	float8 def = PG_GETARG_FLOAT8(2);
	PCSCHEMA *old_schema = pc_schema_from_pcid(serpa->pcid, fcinfo);
	PCSCHEMA *new_schema = pc_schema_from_pcid(pcid, fcinfo);

	if ( pcpatch_schema_same_dimensions(old_schema, new_schema) )
	{
		// old_schema and new_schema have the same dimensions at the same
		// positions, so we can take a fast path avoid the point-by-point,
		// dimension-by-dimension copying

		if ( old_schema->compression == new_schema->compression )
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
			paout = pc_patch_deserialize(serpa, old_schema);
			if ( ! paout )
				PG_RETURN_NULL();
			paout->schema = new_schema;
		}
	} else {
		PCPATCH *patch;

		patch = pc_patch_deserialize(serpa, old_schema);
		if ( ! patch )
			PG_RETURN_NULL();

		paout = pc_patch_set_schema(patch, new_schema, def);

		if ( patch != paout )
			pc_patch_free(patch);

		if ( ! paout )
			PG_RETURN_NULL();

	}

	serpatch = pc_patch_serialize(paout, NULL);
	pc_patch_free(paout);

	PG_RETURN_POINTER(serpatch);
}
