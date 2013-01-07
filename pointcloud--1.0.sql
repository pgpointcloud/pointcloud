
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pointcloud" to load this file. \quit

-- We need this for every point cloud, so we won't bury it in the XML
CREATE DOMAIN pointcloud_compression AS TEXT
CHECK (
    VALUE = 'GHT' OR
    VALUE = 'NONE'
);


-- Metadata table describing contents of pcpoints
CREATE TABLE pointcloud_formats (
    pcid INTEGER PRIMARY KEY,
    srid INTEGER, -- REFERENCE spatial_ref_sys(srid)
    compression pointcloud_compression,
    format XML
);
-- Register pointcloud_formats table so the contents are included in pg_dump output
SELECT pg_catalog.pg_extension_config_dump('pointcloud_formats', '');