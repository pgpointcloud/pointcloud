
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pointcloud" to load this file. \quit


-------------------------------------------------------------------
--  METADATA and SCHEMA
-------------------------------------------------------------------

-- Confirm the XML representation of a schema has everything we need
CREATE OR REPLACE FUNCTION PC_SchemaIsValid(xml text)
	RETURNS boolean AS 'MODULE_PATHNAME','PC_SchemaIsValid'
    LANGUAGE 'c' IMMUTABLE STRICT;

-- Metadata table describing contents of pcpoints
CREATE TABLE pointcloud_formats (
    pcid INTEGER PRIMARY KEY,
    srid INTEGER, -- REFERENCES spatial_ref_sys(srid)
    schema TEXT 
		CHECK ( PC_SchemaIsValid(schema) )
);

-- Register pointcloud_formats table so the contents are included in pg_dump output
SELECT pg_catalog.pg_extension_config_dump('pointcloud_formats', '');

CREATE OR REPLACE FUNCTION PC_SchemaGetNDims(pcid integer)
	RETURNS integer AS 'MODULE_PATHNAME','PC_SchemaGetNDims'
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
	-- typmod_in = geometry_typmod_in,
	-- typmod_out = geometry_typmod_out,
	-- delimiter = ':',
	-- alignment = double,
	-- analyze = geometry_analyze,
	storage = main
);

CREATE OR REPLACE FUNCTION PC_Get(pt pcpoint, dimname text)
	RETURNS numeric AS 'MODULE_PATHNAME', 'PC_Get'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_MakePoint(pcid integer, vals float8[])
	RETURNS pcpoint AS 'MODULE_PATHNAME', 'PC_MakePointFromArray'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_AsText(p pcpoint)
	RETURNS text AS 'MODULE_PATHNAME', 'PC_PointAsText'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION PC_PointAsByteA(p pcpoint)
	RETURNS bytea AS 'MODULE_PATHNAME', 'PC_PointAsByteA'
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
	-- typmod_in = geometry_typmod_in,
	-- typmod_out = geometry_typmod_out,
	-- delimiter = ':',
	-- alignment = double,
	-- analyze = geometry_analyze,
	storage = main
);

CREATE OR REPLACE FUNCTION PC_AsText(p pcpatch)
	RETURNS text AS 'MODULE_PATHNAME', 'PC_PatchAsText'
	LANGUAGE 'c' IMMUTABLE STRICT;



