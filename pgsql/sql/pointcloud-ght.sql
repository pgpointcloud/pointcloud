INSERT INTO pointcloud_formats (pcid, srid, schema)
VALUES (5, 0, 
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
    <Metadata name="compression">ght</Metadata>
    <Metadata name="spatialreference" type="id">4326</Metadata>
  </pc:metadata>
</pc:PointCloudSchema>'
);


CREATE TABLE IF NOT EXISTS pa_test_ght (
    pa PCPATCH(5)
);
\d pa_test_ght

INSERT INTO pa_test_ght (pa) VALUES ('0000000005000000000000000200000002000000030000000500060000000200000003000000050008');
INSERT INTO pa_test_ght (pa) VALUES ('000000000500000000000000020000000600000007000000050006000000090000000A00000005000A');
INSERT INTO pa_test_ght (pa) VALUES ('0000000005000000000000000200000002000000030000000500060000000200000003000000050003');
INSERT INTO pa_test_ght (pa) VALUES ('0000000005000000000000000200000002000000030000000500060000000200000003000000050001');

SELECT Sum(PC_NumPoints(pa)) FROM pa_test_ght;
SELECT Sum(PC_MemSize(pa)) FROM pa_test_ght;
SELECT Sum(PC_PatchMax(pa,'x')) FROM pa_test_ght;
SELECT Sum(PC_PatchMin(pa,'x')) FROM pa_test_ght;

DELETE FROM pa_test_ght;
INSERT INTO pa_test_ght (pa)
SELECT PC_Patch(PC_MakePoint(5, ARRAY[x,y,z,intensity]))
FROM (
 SELECT  
 -127+a/100.0 AS x, 
   45+a/100.0 AS y,
        1.0*a AS z,
         a/10 AS intensity,
         a/400 AS gid
 FROM generate_series(1,1600) AS a
) AS values GROUP BY gid;

SELECT Sum(PC_NumPoints(pa)) FROM pa_test_ght;
SELECT Sum(PC_MemSize(pa)) FROM pa_test_ght;

SELECT Max(PC_PatchMax(pa,'x')) FROM pa_test_ght;
SELECT Min(PC_PatchMin(pa,'x')) FROM pa_test_ght;
SELECT Min(PC_PatchMin(pa,'z')) FROM pa_test_ght;

TRUNCATE pointcloud_formats;
