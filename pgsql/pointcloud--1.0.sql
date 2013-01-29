
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




-------------------------------------------------------------------
--  SAMPLE DATA
--

INSERT INTO pointcloud_formats (pcid, srid, schema) VALUES (1, 4326, '<?xml version="1.0" encoding="UTF-8"?>
<pc:PointCloudSchema xmlns:pc="http://pointcloud.org/schemas/PC/1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <pc:dimension>
    <pc:position>1</pc:position>
    <pc:size>4</pc:size>
    <pc:description>X coordinate as a long integer. You must use the scale and offset information of the header to determine the double value.</pc:description>
    <pc:name>X</pc:name>
    <pc:interpretation>int32_t</pc:interpretation>
    <pc:scale>0.01</pc:scale>
  </pc:dimension>
  <pc:dimension>
    <pc:position>2</pc:position>
    <pc:size>4</pc:size>
    <pc:description>Y coordinate as a long integer. You must use the scale and offset information of the header to determine the double value.</pc:description>
    <pc:name>Y</pc:name>
    <pc:interpretation>int32_t</pc:interpretation>
    <pc:scale>0.01</pc:scale>
  </pc:dimension>
  <pc:dimension>
    <pc:position>3</pc:position>
    <pc:size>4</pc:size>
    <pc:description>Z coordinate as a long integer. You must use the scale and offset information of the header to determine the double value.</pc:description>
    <pc:name>Z</pc:name>
    <pc:interpretation>int32_t</pc:interpretation>
    <pc:scale>0.01</pc:scale>
  </pc:dimension>
  <pc:dimension>
    <pc:position>4</pc:position>
    <pc:size>2</pc:size>
    <pc:description>The intensity value is the integer representation of the pulse return magnitude. This value is optional and system specific. However, it should always be included if available.</pc:description>
    <pc:name>Intensity</pc:name>
    <pc:interpretation>uint16_t</pc:interpretation>
    <pc:scale>1</pc:scale>
  </pc:dimension>
  <pc:metadata>
    <Metadata name="compression"></Metadata>
    <Metadata name="ght_xmin"></Metadata>
    <Metadata name="ght_ymin"></Metadata>
    <Metadata name="ght_xmax"></Metadata>
    <Metadata name="ght_ymax"></Metadata>
    <Metadata name="ght_keylength"></Metadata>
    <Metadata name="ght_depth"></Metadata>
    <Metadata name="spatialreference" type="id" authority="EPSG">4326</Metadata>
  </pc:metadata>
</pc:PointCloudSchema>
');