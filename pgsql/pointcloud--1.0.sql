
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pointcloud" to load this file. \quit

-- Metadata table describing contents of pcpoints
CREATE TABLE pointcloud_formats (
    pcid INTEGER PRIMARY KEY,
    srid INTEGER, -- REFERENCES spatial_ref_sys(srid)
    schema XML
);
-- Register pointcloud_formats table so the contents are included in pg_dump output
SELECT pg_catalog.pg_extension_config_dump('pointcloud_formats', '');