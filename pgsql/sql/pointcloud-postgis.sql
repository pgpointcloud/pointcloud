
CREATE EXTENSION postgis;

-- Turn points into patches
DROP TABLE IF EXISTS pch_collection;
CREATE TABLE pch_collection AS
SELECT gid, PC_Patch(pt)::pcpatch(1) AS pch FROM pts_collection GROUP BY gid;

-- Utility function to overlap polygon on patch
CREATE OR REPLACE FUNCTION PC_Intersection(pcpatch, geometry)
RETURNS pcpatch AS
$$
WITH 
     pts AS (SELECT PC_Explode($1) AS pt), 
   pgpts AS (SELECT ST_GeomFromEWKB(PC_AsBinary(pt)) AS pgpt, pt FROM pts),
    ipts AS (SELECT pt FROM pgpts WHERE ST_Intersects(pgpt, $2)),
    ipch AS (SELECT PC_Patch(pt) AS pch FROM ipts)
SELECT pch FROM ipch;
$$ LANGUAGE 'sql';

-- Utility cast from patch to polygon
CREATE OR REPLACE FUNCTION pcpatch2geometry(pcpatch)
RETURNS geometry AS
$$
SELECT ST_GeomFromEWKB(PC_Envelope($1))
$$ LANGUAGE 'sql';

CREATE CAST (pcpatch AS geometry) WITH FUNCTION pcpatch2geometry(pcpatch);


-- Now create spatial index with cast
CREATE INDEX pch_gix ON pch_collection USING GIST ( pcpatch2geometry(pch) );

-- Use the index
SELECT Count(*) FROM pch_collection WHERE
pcpatch2geometry(pch) && 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))';

-- Use the polygon overlay
SELECT PC_AsText(PC_Intersection(pch, 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))'))
FROM pch_collection WHERE
pcpatch2geometry(pch) && 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))';


-- Use the polygon overlay
SELECT PC_AsText(PC_Explode(PC_Intersection(pch, 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))')))
FROM pch_collection WHERE
pcpatch2geometry(pch) && 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))';

-- Use a filter
SELECT PC_Get(PC_Explode(PC_Intersection(pch, 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))')),'z') AS z
FROM pch_collection WHERE
pcpatch2geometry(pch) && 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))';


WITH zs AS (
SELECT PC_Get(PC_Explode(PC_Intersection(pch, 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))')),'z') AS z
FROM pch_collection WHERE
pcpatch2geometry(pch) && 'POLYGON((10 10, 10 11, 11 11, 11 10, 10 10))'
)
SELECT avg(z) FROM zs;