-----------------------------------------------------------------------------
-- Function to overlap polygon on patch
--
CREATE OR REPLACE FUNCTION PC_Intersection(pcpatch, geometry)
    RETURNS pcpatch AS
    $$
        WITH 
             pts AS (SELECT PC_Explode($1) AS pt), 
           pgpts AS (SELECT ST_GeomFromEWKB(PC_AsBinary(pt)) AS pgpt, pt FROM pts),
            ipts AS (SELECT pt FROM pgpts WHERE ST_Intersects(pgpt, $2)),
            ipch AS (SELECT PC_Patch(pt) AS pch FROM ipts)
        SELECT pch FROM ipch;
    $$ 
    LANGUAGE 'sql';

-----------------------------------------------------------------------------
-- Cast from pcpatch to polygon
--
CREATE OR REPLACE FUNCTION pcpatch2geometry(pcpatch)
    RETURNS geometry AS
    $$
        SELECT ST_GeomFromEWKB(PC_Envelope($1))
    $$ 
    LANGUAGE 'sql';

CREATE CAST (pcpatch AS geometry) WITH FUNCTION pcpatch2geometry(pcpatch);

-----------------------------------------------------------------------------
-- Cast from pcpoint to point
--
CREATE OR REPLACE FUNCTION pcpoint2geometry(pcpoint)
    RETURNS geometry AS
    $$
        SELECT ST_GeomFromEWKB(PC_AsBinary($1))
    $$ 
    LANGUAGE 'sql';

CREATE CAST (pcpoint AS geometry) WITH FUNCTION pcpoint2geometry(pcpoint);

