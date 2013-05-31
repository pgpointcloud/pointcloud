
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pointcloud" to load this file. \quit


-------------------------------------------------------------------
--  METADATA and SCHEMA
-------------------------------------------------------------------

-- Confirm the XML representation of a schema has everything we need
CREATE OR REPLACE FUNCTION PC_SchemaIsValid(schemaxml text)
	RETURNS boolean AS 'MODULE_PATHNAME','pcschema_is_valid'
    LANGUAGE 'c' IMMUTABLE STRICT;

-- Metadata table describing contents of pcpoints
CREATE TABLE pointcloud_formats (
    pcid INTEGER PRIMARY KEY 
        -- PCID == 0 is unknown
        -- PCID > 2^16 is reserved to leave space in typmod
        CHECK (pcid > 0 AND pcid < 65536),
    srid INTEGER, -- REFERENCES spatial_ref_sys(srid)
    schema TEXT 
		CHECK ( PC_SchemaIsValid(schema) )
);

-- Register pointcloud_formats table so the contents are included in pg_dump output
SELECT pg_catalog.pg_extension_config_dump('pointcloud_formats', '');

CREATE OR REPLACE FUNCTION PC_SchemaGetNDims(pcid integer)
	RETURNS integer 
	AS 'MODULE_PATHNAME','pcschema_get_ndims'
    LANGUAGE 'c' IMMUTABLE STRICT;

-- Read typmod number from string
CREATE OR REPLACE FUNCTION pc_typmod_in(cstring[])
    RETURNS integer AS 'MODULE_PATHNAME','pc_typmod_in'
    LANGUAGE 'c' IMMUTABLE STRICT;

-- Write typmod number to string
CREATE OR REPLACE FUNCTION pc_typmod_out(typmod integer)
    RETURNS cstring AS 'MODULE_PATHNAME','pc_typmod_out'
    LANGUAGE 'c' IMMUTABLE STRICT;

-- Read pcid from typmod number
CREATE OR REPLACE FUNCTION pc_typmod_pcid(typmod integer)
    RETURNS int4 AS 'MODULE_PATHNAME','pc_typmod_pcid'
    LANGUAGE 'c' IMMUTABLE STRICT;

-- Return the library version number
CREATE OR REPLACE FUNCTION pc_version()
    RETURNS text AS 'MODULE_PATHNAME', 'pc_version'
    LANGUAGE 'c' IMMUTABLE STRICT;

-------------------------------------------------------------------
--  PCPOINT
-------------------------------------------------------------------

CREATE OR REPLACE FUNCTION pcpoint_in(cstring)
	RETURNS pcpoint AS 'MODULE_PATHNAME', 'pcpoint_in'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pcpoint_out(pcpoint)
	RETURNS cstring AS 'MODULE_PATHNAME', 'pcpoint_out'
	LANGUAGE 'c' IMMUTABLE STRICT;
	
CREATE TYPE pcpoint (
	internallength = variable,
	input = pcpoint_in,
	output = pcpoint_out,
	-- send = geometry_send,
	-- receive = geometry_recv,
	typmod_in = pc_typmod_in,
	typmod_out = pc_typmod_out,
	-- delimiter = ':',
	-- alignment = double,
	-- analyze = geometry_analyze,
	storage = external -- do not try to compress it please
);

CREATE OR REPLACE FUNCTION PC_Get(pt pcpoint, dimname text)
	RETURNS numeric AS 'MODULE_PATHNAME', 'pcpoint_get_value'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_MakePoint(pcid integer, vals float8[])
	RETURNS pcpoint AS 'MODULE_PATHNAME', 'pcpoint_from_double_array'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_AsText(p pcpoint)
	RETURNS text AS 'MODULE_PATHNAME', 'pcpoint_as_text'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_AsBinary(p pcpoint)
	RETURNS bytea AS 'MODULE_PATHNAME', 'pcpoint_as_bytea'
	LANGUAGE 'c' IMMUTABLE STRICT;

-------------------------------------------------------------------
--  PCPATCH
-------------------------------------------------------------------

CREATE OR REPLACE FUNCTION pcpatch_in(cstring)
	RETURNS pcpatch AS 'MODULE_PATHNAME', 'pcpatch_in'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pcpatch_out(pcpatch)
	RETURNS cstring AS 'MODULE_PATHNAME', 'pcpatch_out'
	LANGUAGE 'c' IMMUTABLE STRICT;
	
CREATE TYPE pcpatch (
	internallength = variable,
	input = pcpatch_in,
	output = pcpatch_out,
	-- send = geometry_send,
	-- receive = geometry_recv,
	typmod_in = pc_typmod_in,
	typmod_out = pc_typmod_out,
	-- delimiter = ':',
	-- alignment = double,
	-- analyze = geometry_analyze,
	storage = main
);

CREATE OR REPLACE FUNCTION PC_AsText(p pcpatch)
	RETURNS text AS 'MODULE_PATHNAME', 'pcpatch_as_text'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_Envelope(p pcpatch)
	RETURNS bytea AS 'MODULE_PATHNAME', 'pcpatch_bytea_envelope'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_Uncompress(p pcpatch)
    RETURNS pcpatch AS 'MODULE_PATHNAME', 'pcpatch_uncompress'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_NumPoints(p pcpatch)
    RETURNS int4 AS 'MODULE_PATHNAME', 'pcpatch_numpoints'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_Compression(p pcpatch)
    RETURNS int4 AS 'MODULE_PATHNAME', 'pcpatch_compression'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_Intersects(p1 pcpatch, p2 pcpatch)
    RETURNS boolean AS 'MODULE_PATHNAME', 'pcpatch_intersects'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_MemSize(p pcpatch)
    RETURNS int4 AS 'MODULE_PATHNAME', 'pcpatch_size'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_MemSize(p pcpoint)
    RETURNS int4 AS 'MODULE_PATHNAME', 'pcpoint_size'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_PatchMax(p pcpatch, attr text, stat text default 'max')
	RETURNS numeric AS 'MODULE_PATHNAME', 'pcpatch_get_stat'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_PatchMin(p pcpatch, attr text, stat text default 'min')
	RETURNS numeric AS 'MODULE_PATHNAME', 'pcpatch_get_stat'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_PatchAvg(p pcpatch, attr text, stat text default 'avg')
	RETURNS numeric AS 'MODULE_PATHNAME', 'pcpatch_get_stat'
    LANGUAGE 'c' IMMUTABLE STRICT;

-------------------------------------------------------------------
--  POINTCLOUD_COLUMNS
-------------------------------------------------------------------

CREATE OR REPLACE VIEW pointcloud_columns AS
    SELECT
        n.nspname AS schema, 
        c.relname AS table, 
        a.attname AS column,
        pc_typmod_pcid(a.atttypmod) AS pcid,
        p.srid AS srid,
        t.typname AS type
    FROM 
        pg_class c, 
        pg_attribute a, 
        pg_type t, 
        pg_namespace n,
        pointcloud_formats p
    WHERE t.typname IN ('pcpatch','pcpoint')
    AND a.attisdropped = false
    AND a.atttypid = t.oid
    AND a.attrelid = c.oid
    AND c.relnamespace = n.oid
    AND (pc_typmod_pcid(a.atttypmod) = p.pcid OR pc_typmod_pcid(a.atttypmod) IS NULL)
    AND NOT pg_is_other_temp_schema(c.relnamespace)
    AND has_table_privilege( c.oid, 'SELECT'::text );


-- Special cast for enforcing typmod restrictions
CREATE OR REPLACE FUNCTION pcpatch(p pcpatch, typmod integer, explicit boolean)
    RETURNS pcpatch AS 'MODULE_PATHNAME', 'pcpatch_enforce_typmod'
    LANGUAGE 'c' IMMUTABLE STRICT; 

CREATE CAST (pcpatch AS pcpatch) WITH FUNCTION pcpatch(pcpatch, integer, boolean) AS IMPLICIT;

CREATE OR REPLACE FUNCTION pcpoint(p pcpoint, typmod integer, explicit boolean)
    RETURNS pcpoint AS 'MODULE_PATHNAME', 'pcpoint_enforce_typmod'
    LANGUAGE 'c' IMMUTABLE STRICT; 

CREATE CAST (pcpoint AS pcpoint) WITH FUNCTION pcpoint(pcpoint, integer, boolean) AS IMPLICIT;

-------------------------------------------------------------------
--  AGGREGATE GENERIC SUPPORT
-------------------------------------------------------------------

CREATE OR REPLACE FUNCTION pointcloud_abs_in(cstring)
	RETURNS pointcloud_abs AS 'MODULE_PATHNAME'
	LANGUAGE 'c';

CREATE OR REPLACE FUNCTION pointcloud_abs_out(pointcloud_abs)
	RETURNS cstring AS 'MODULE_PATHNAME'
	LANGUAGE 'c';

CREATE TYPE pointcloud_abs (
	internallength = 8,
	input = pointcloud_abs_in,
	output = pointcloud_abs_out,
	alignment = double
);

-------------------------------------------------------------------
--  AGGREGATE PCPOINT
-------------------------------------------------------------------

CREATE OR REPLACE FUNCTION PC_Patch(pcpoint[])
	RETURNS pcpatch AS 'MODULE_PATHNAME', 'pcpatch_from_pcpoint_array'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pcpoint_agg_transfn (pointcloud_abs, pcpoint)
	RETURNS pointcloud_abs AS 'MODULE_PATHNAME', 'pointcloud_agg_transfn'
	LANGUAGE 'c';

CREATE OR REPLACE FUNCTION pcpoint_agg_final_array (pointcloud_abs)
	RETURNS pcpoint[] AS 'MODULE_PATHNAME', 'pcpoint_agg_final_array'
	LANGUAGE 'c';

CREATE OR REPLACE FUNCTION pcpoint_agg_final_pcpatch (pointcloud_abs)
	RETURNS pcpatch AS 'MODULE_PATHNAME', 'pcpoint_agg_final_pcpatch'
	LANGUAGE 'c';

CREATE AGGREGATE PC_Patch (
        BASETYPE = pcpoint,
        SFUNC = pcpoint_agg_transfn,
        STYPE = pointcloud_abs,
        FINALFUNC = pcpoint_agg_final_pcpatch
);

CREATE AGGREGATE PC_Point_Agg (
        BASETYPE = pcpoint,
        SFUNC = pcpoint_agg_transfn,
        STYPE = pointcloud_abs,
        FINALFUNC = pcpoint_agg_final_array
);

-------------------------------------------------------------------
--  AGGREGATE / EXPLODE PCPATCH
-------------------------------------------------------------------

CREATE OR REPLACE FUNCTION pcpatch_agg_final_array (pointcloud_abs)
	RETURNS pcpatch[] AS 'MODULE_PATHNAME', 'pcpatch_agg_final_array'
	LANGUAGE 'c';

CREATE OR REPLACE FUNCTION pcpatch_agg_final_pcpatch (pointcloud_abs)
	RETURNS pcpatch AS 'MODULE_PATHNAME', 'pcpatch_agg_final_pcpatch'
	LANGUAGE 'c';

CREATE OR REPLACE FUNCTION pcpatch_agg_transfn (pointcloud_abs, pcpatch)
	RETURNS pointcloud_abs AS 'MODULE_PATHNAME', 'pointcloud_agg_transfn'
	LANGUAGE 'c';
	
CREATE AGGREGATE PC_Patch_Agg (
        BASETYPE = pcpatch,
        SFUNC = pcpatch_agg_transfn,
        STYPE = pointcloud_abs,
        FINALFUNC = pcpatch_agg_final_array
);

CREATE AGGREGATE PC_Union (
        BASETYPE = pcpatch,
        SFUNC = pcpatch_agg_transfn,
        STYPE = pointcloud_abs,
        FINALFUNC = pcpatch_agg_final_pcpatch
);

CREATE OR REPLACE FUNCTION PC_Explode(p pcpatch)
	RETURNS setof pcpoint AS 'MODULE_PATHNAME', 'pcpatch_unnest'
	LANGUAGE 'c' IMMUTABLE STRICT;


-------------------------------------------------------------------
--  SQL Utility Functions
-------------------------------------------------------------------

