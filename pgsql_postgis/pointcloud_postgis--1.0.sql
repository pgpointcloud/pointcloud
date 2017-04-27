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
CREATE OR REPLACE FUNCTION PC_EnvelopeGeometry(pcpatch)
	RETURNS geometry AS
	$$
		SELECT ST_GeomFromEWKB(PC_EnvelopeAsBinary($1))
	$$
	LANGUAGE 'sql';

CREATE OR REPLACE FUNCTION geometry(pcpatch)
	RETURNS geometry AS
	$$
		SELECT PC_EnvelopeGeometry($1)
	$$
	LANGUAGE 'sql';

CREATE CAST (pcpatch AS geometry) WITH FUNCTION PC_EnvelopeGeometry(pcpatch);


-----------------------------------------------------------------------------
-- Cast from pcpoint to point
--
CREATE OR REPLACE FUNCTION geometry(pcpoint)
	RETURNS geometry AS
	$$
		SELECT ST_GeomFromEWKB(PC_AsBinary($1))
	$$
	LANGUAGE 'sql';

CREATE CAST (pcpoint AS geometry) WITH FUNCTION geometry(pcpoint);


-----------------------------------------------------------------------------
-- Function to overlap polygon on patch
--
CREATE OR REPLACE FUNCTION PC_Intersects(pcpatch, geometry)
	RETURNS boolean AS
	$$
		SELECT ST_Intersects($2, PC_EnvelopeGeometry($1))
	$$
	LANGUAGE 'sql';

CREATE OR REPLACE FUNCTION PC_Intersects(geometry, pcpatch)
	RETURNS boolean AS
	$$
		SELECT PC_Intersects($2, $1)
	$$
	LANGUAGE 'sql';

-----------------------------------------------------------------------------
-- Function from pcpatch to LineString
--
CREATE OR REPLACE FUNCTION PC_BoundingDiagonalGeometry(pcpatch)
	RETURNS geometry AS
	$$
		SELECT ST_GeomFromEWKB(PC_BoundingDiagonalAsBinary($1))
	$$
	LANGUAGE 'sql';
