CREATE EXTENSION pointcloud;

SELECT PC_Version();

INSERT INTO pointcloud_formats (pcid, srid, schema)
VALUES (1, 0, -- XYZI, scaled, uncompressed
'<?xml version="1.0" encoding="UTF-8"?>
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
    <Metadata name="compression">none</Metadata>
    <Metadata name="ght_xmin"></Metadata>
    <Metadata name="ght_ymin"></Metadata>
    <Metadata name="ght_xmax"></Metadata>
    <Metadata name="ght_ymax"></Metadata>
    <Metadata name="ght_keylength"></Metadata>
    <Metadata name="ght_depth"></Metadata>
    <Metadata name="spatialreference" type="id">4326</Metadata>
  </pc:metadata>
</pc:PointCloudSchema>'
),
(3, 0, -- XYZI, scaled, dimensionally compressed
'<?xml version="1.0" encoding="UTF-8"?>
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
    <Metadata name="compression">dimensional</Metadata>
    <Metadata name="spatialreference" type="id">4326</Metadata>
  </pc:metadata>
</pc:PointCloudSchema>'
),
(20, 0, -- XYZ, unscaled, dimensionally compressed
'<?xml version="1.0" encoding="UTF-8"?>
<pc:PointCloudSchema xmlns:pc="http://pointcloud.org/schemas/PC/1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <pc:dimension>
    <pc:position>1</pc:position>
    <pc:size>4</pc:size>
    <pc:name>X</pc:name>
    <pc:interpretation>int32_t</pc:interpretation>
  </pc:dimension>
  <pc:dimension>
    <pc:position>2</pc:position>
    <pc:size>4</pc:size>
    <pc:name>Y</pc:name>
    <pc:interpretation>int32_t</pc:interpretation>
  </pc:dimension>
  <pc:dimension>
    <pc:position>3</pc:position>
    <pc:size>4</pc:size>
    <pc:name>Z</pc:name>
    <pc:interpretation>int32_t</pc:interpretation>
  </pc:dimension>
  <pc:metadata>
    <Metadata name="compression">dimensional</Metadata>
  </pc:metadata>
</pc:PointCloudSchema>'
);

CREATE TABLE IF NOT EXISTS pt_test (
    pt PCPOINT(1)
);
\d pt_test

DELETE FROM pt_test;
INSERT INTO pt_test (pt) VALUES ('00000000020000000100000002000000030004');
INSERT INTO pt_test (pt) VALUES ('00000000010000000100000002000000030004');
INSERT INTO pt_test (pt) VALUES ('00000000010000000200000003000000030005');
INSERT INTO pt_test (pt) VALUES ('00000000010000000300000004000000030006');

SELECT PC_Get(pt, 'Intensity') FROM pt_test;
SELECT Sum(PC_Get(pt, 'y')) FROM pt_test;

SELECT PC_AsText(pt) FROM pt_test;

SELECT PC_AsText(PC_Patch(pt)) FROM pt_test;
SELECT PC_AsText(PC_Explode(PC_Patch(pt))) FROM pt_test;
SELECT Sum(PC_MemSize(pt)) FROM pt_test;

CREATE TABLE IF NOT EXISTS pa_test (
    pa PCPATCH(1)
);
\d pa_test

DELETE FROM pa_test;
INSERT INTO pa_test (pa) VALUES ('0000000002000000000000000200000002000000030000000500060000000200000003000000050008');
INSERT INTO pa_test (pa) VALUES ('0000000001000000000000000200000002000000030000000500060000000200000003000000050008');
INSERT INTO pa_test (pa) VALUES ('000000000100000000000000020000000600000007000000050006000000090000000A00000005000A');
INSERT INTO pa_test (pa) VALUES ('000000000100000000000000020000000600000007000000050006000000090000000A00000005000A');
INSERT INTO pa_test (pa) VALUES ('000000000100000000000000020000000600000007000000050006000000090000000A00000005000A');

SELECT PC_Uncompress(pa) FROM pa_test LIMIT 1;

SELECT PC_AsText(pa) FROM pa_test;
SELECT PC_Envelope(pa) from pa_test;
SELECT PC_AsText(PC_Union(pa)) FROM pa_test;
SELECT sum(PC_NumPoints(pa)) FROM pa_test;

CREATE TABLE IF NOT EXISTS pa_test_dim (
    pa PCPATCH(3)
);
\d pa_test_dim

INSERT INTO pa_test_dim (pa) VALUES ('0000000003000000000000000200000002000000030000000500060000000200000003000000050008');
INSERT INTO pa_test_dim (pa) VALUES ('000000000300000000000000020000000600000007000000050006000000090000000A00000005000A');
INSERT INTO pa_test_dim (pa) VALUES ('0000000003000000000000000200000002000000030000000500060000000200000003000000050003');
INSERT INTO pa_test_dim (pa) VALUES ('0000000003000000000000000200000002000000030000000500060000000200000003000000050001');

SELECT Sum(PC_NumPoints(pa)) FROM pa_test_dim;
SELECT Sum(PC_MemSize(pa)) FROM pa_test_dim;
SELECT Sum(PC_PatchMax(pa,'x')) FROM pa_test_dim;
SELECT Sum(PC_PatchMin(pa,'x')) FROM pa_test_dim;

DELETE FROM pa_test_dim;
INSERT INTO pa_test_dim (pa)
SELECT PC_Patch(PC_MakePoint(3, ARRAY[x,y,z,intensity]))
FROM (
 SELECT  
 -127+a/100.0 AS x, 
   45+a/100.0 AS y,
        1.0*a AS z,
         a/10 AS intensity,
         a/400 AS gid
 FROM generate_series(1,1600) AS a
) AS values GROUP BY gid;

SELECT Sum(PC_NumPoints(pa)) FROM pa_test_dim;
SELECT Sum(PC_MemSize(pa)) FROM pa_test_dim;

SELECT Max(PC_PatchMax(pa,'x')) FROM pa_test_dim;
SELECT Min(PC_PatchMin(pa,'x')) FROM pa_test_dim;
SELECT Min(PC_PatchMin(pa,'z')) FROM pa_test_dim;



--DROP TABLE pts_collection;
DROP TABLE pt_test;
DROP TABLE pa_test;
DROP TABLE pa_test_dim;

-- See https://github.com/pgpointcloud/pointcloud/issues/44
SELECT PC_AsText(PC_Patch(ARRAY[PC_MakePoint(3, ARRAY[-127, 45, 124.0, 4.0])]::pcpoint[]));

-- https://github.com/pgpointcloud/pointcloud/issues/79
SELECT '#79' issue,
  PC_PatchMin(p,'x') x_min, PC_PatchMax(p,'x') x_max,
  PC_PatchMin(p,'y') y_min, PC_PatchMax(p,'y') y_max,
  PC_PatchMin(p,'z') z_min, PC_PatchMax(p,'z') z_max
FROM ( SELECT
  PC_FilterEquals(
    PC_Patch( PC_MakePoint(20,ARRAY[-1,0,1]) ),
    'y',0) p
) foo;

-- https://github.com/pgpointcloud/pointcloud/issues/78
SELECT '#78' issue,
  PC_PatchMin(p,'x') x_min, PC_PatchMax(p,'x') x_max,
  PC_PatchMin(p,'y') y_min, PC_PatchMax(p,'y') y_max,
  PC_PatchMin(p,'z') z_min, PC_PatchMax(p,'z') z_max,
  PC_PatchMin(p,'intensity') i_min, PC_PatchMax(p,'intensity') i_max
FROM ( SELECT
  PC_FilterEquals(
    PC_Patch( PC_MakePoint(3,ARRAY[-1,0,4862413,1]) ),
    'y',0) p
) foo;

TRUNCATE pointcloud_formats;
