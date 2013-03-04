
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pointcloud" to load this file. \quit


-------------------------------------------------------------------
--  METADATA and SCHEMA
-------------------------------------------------------------------

-- Confirm the XML representation of a schema has everything we need
CREATE OR REPLACE FUNCTION PC_SchemaIsValid(xml text)
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
CREATE OR REPLACE FUNCTION pc_typmod_out(integer)
    RETURNS cstring AS 'MODULE_PATHNAME','pc_typmod_out'
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
	storage = main
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


CREATE OR REPLACE FUNCTION PC_Explode(pcpatch)
	RETURNS setof pcpoint AS 'MODULE_PATHNAME', 'pcpatch_unnest'
	LANGUAGE 'c' IMMUTABLE STRICT;
	

