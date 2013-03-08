# Pointcloud #

A PostgreSQL extension for storing point cloud (LIDAR) data.


## Build/Install ##

### Requirements ###

- PostgreSQL and PostgreSQL development packages must be installed (pg_config and server headers). For Red Hat and Ubuntu, the package names are usually "postgresql-dev" or "postgresql-devel"
- LibXML2 development packages must be installed, usually "libxml2-dev" or "libxml2-devel".
- CUnit packages must be installed, or [source built and installed](http://sourceforge.net/projects/cunit/ "CUnit").


### Build ###

- `./configure`
- `make`
- `sudo make install`


### Activate ###

- `CREATE DATABASE mynewdb`
- `CREATE EXTENSION pointcloud`

## Schemas ##

LIDAR sensors quickly produce millions of points with large numbers of variables measured on each point. The challenge for a point cloud database extension is efficiently storing this data while allowing high fidelity access to the many variables stored. 

Much of the complexity in handling LIDAR comes from the need to deal with multiple variables per point. The variables captured by LIDAR sensors varies by sensor and capture process. Some data sets might contain only X/Y/Z values. Others will contain dozens of variables: X, Y, Z; intensity and return number; red, green, and blue values; return times; and many more. There is no consistency in how variables are stored: intensity might be stored in a 4-byte integer, or in a single byte; X/Y/Z might be doubles, or they might be scaled 4-byte integers. 

PostgreSQL Pointcloud deals with all this variability by using a "schema document" to describe the contents of any particular LIDAR point. Each point contains a number of dimensions, and each dimension can be of any data type, with scaling and/or offsets applied to move between the actual value and the value stored in the database. The schema document format used by PostgreSQL Pointcloud is the same one used by the [PDAL](http://pointcloud.org) library.

Here is a simple 4-dimensional schema document you can insert into `pointcloud_formats` to work with the examples below:

    INSERT INTO pointcloud_formats (pcid, srid, schema) VALUES (1, 4326, 
    '<?xml version="1.0" encoding="UTF-8"?>
    <pc:PointCloudSchema xmlns:pc="http://pointcloud.org/schemas/PC/1.1" 
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
      <pc:dimension>
        <pc:position>1</pc:position>
        <pc:size>4</pc:size>
        <pc:description>X coordinate as a long integer. You must use the 
                        scale and offset information of the header to 
                        determine the double value.</pc:description>
        <pc:name>X</pc:name>
        <pc:interpretation>int32_t</pc:interpretation>
        <pc:scale>0.01</pc:scale>
      </pc:dimension>
      <pc:dimension>
        <pc:position>2</pc:position>
        <pc:size>4</pc:size>
        <pc:description>Y coordinate as a long integer. You must use the 
                        scale and offset information of the header to 
                        determine the double value.</pc:description>
        <pc:name>Y</pc:name>
        <pc:interpretation>int32_t</pc:interpretation>
        <pc:scale>0.01</pc:scale>
      </pc:dimension>
      <pc:dimension>
        <pc:position>3</pc:position>
        <pc:size>4</pc:size>
        <pc:description>Z coordinate as a long integer. You must use the 
                        scale and offset information of the header to 
                        determine the double value.</pc:description>
        <pc:name>Z</pc:name>
        <pc:interpretation>int32_t</pc:interpretation>
        <pc:scale>0.01</pc:scale>
      </pc:dimension>
      <pc:dimension>
        <pc:position>4</pc:position>
        <pc:size>2</pc:size>
        <pc:description>The intensity value is the integer representation 
                        of the pulse return magnitude. This value is optional 
                        and system specific. However, it should always be 
                        included if available.</pc:description>
        <pc:name>Intensity</pc:name>
        <pc:interpretation>uint16_t</pc:interpretation>
        <pc:scale>1</pc:scale>
      </pc:dimension>
      <pc:metadata>
        <Metadata name="compression">dimensional</Metadata>
        <Metadata name="spatialreference" type="id">4326</Metadata>
      </pc:metadata>
    </pc:PointCloudSchema>');

Schema documents are stored in the `pointcloud_formats` table, along with a `pcid` or "pointcloud identifier". Rather than store the whole schema information with each database object, each object just has a `pcid`, which serves as a key to find the schema in `pointcloud_formats`.  This is similar to the way the `srid` is resolved for spatial reference system support in [PostGIS](http://postgis.net).

The central role of the schema document in interpreting the contents of a point cloud object means that care must be taken to ensure that the right `pcid` reference is being used in objects, and that it references a valid schema document in the `pointcloud_formats` table.


## Point Cloud Objects ##

### PcPoint ###

The basic point cloud type is a `PcPoint`. Every point has a (large?) number of dimensions, but at a minimum an X and Y coordinate that place it in space. 

Points can be rendered in a human-readable JSON form using the `PC_AsText(pcpoint)` function. The "pcid" is the foreign key reference to the `pointcloud_formats` table, where the meaning of each dimension in the "pt" array of doubles is explained. The underlying storage of the data might not be double, but by the time it has been extracted, scaled and offset, it is representable as doubles.

    {
        "pcid" : 1,
          "pt" : [0.01, 0.02, 0.03, 4]
    }


### PcPatch ###

The structure of database storage is such that storing billions of points as individual records in a table is not an efficient use of resources. Instead, we collect a group of `PcPoint` into a `PcPatch`. Each patch should hopefully contain points that are near together. 

Instead of a table of billions of single `PcPoint` records, a collection of LIDAR data can be represented in the database as a much smaller collection (10s of millions) of `PcPatch` records. 

Patches can be rendered into a human-readable JSON form using the `PC_AsText(pcpatch)` function.  The "pcid" is the foreign key reference to the `pointcloud_formats` table.

    {
        "pcid" : 1,
         "pts" : [
                  [0.02, 0.03, 0.05, 6],
                  [0.02, 0.03, 0.05, 8]
                 ]
    }

## Tables ##

Usually you will only be creating tables for storing `PcPatch` objects, and using `PcPoint` objects as transitional objects for filtering, but it is possible to create tables of both types. `PcPatch` and `PcPoint` columns both require an argument that indicate the `pcid` that will be used to interpret the column. 

    -- This example requires the schema entry from the previous 
    -- section to be loaded so that pcid==1 exists.
    
    -- A table of points
    CREATE TABLE points (
        id SERIAL PRIMARY KEY,
        pt PCPOINT(1)
    );

    -- A table of patches
    CREATE TABLE patches (
        id SERIAL PRIMARY KEY,
        pa PCPATCH(1)
    );

In addition to any tables you create, you will find two other system-provided point cloud tables,

- the `pointcloud_formats` table that holds all the pcid entries and schema documents
- the `pointcloud_columns` view, that displays all the columns in your database that contain point cloud objects

Now that you have created two tables, you'll see entries for them in the `pointcloud_columns` view:

    SELECT * FROM pointcloud_columns;

     schema |    table    | column | pcid | srid |  type   
    --------+-------------+--------+------+------+---------
     public | points      | pt     |    1 | 4326 | pcpoint
     public | patches     | pa     |    1 | 4326 | pcpatch


## Functions ##

**PC_MakePoint(pcid integer, vals float8[])** returns **pcpoint**

> Given a valid `pcid` schema number and an array of doubles that matches the schema, construct a new `pcpoint`.
> 
>     SELECT PC_MakePoint(1, ARRAY[-127, 45, 124.0, 4.0]);
>
>     010100000064CEFFFF94110000703000000400
>
> Insert some test values into the `points` table.
>
>     INSERT INTO points (pt)
>     SELECT PC_MakePoint(1, ARRAY[x,y,z,intensity])
>     FROM (
>       SELECT  
>       -127+a/100.0 AS x, 
>         45+a/100.0 AS y,
>              1.0*a AS z,
>               a/10 AS intensity
>       FROM generate_series(1,100) AS a
>     ) AS values;

**PC_AsText(p pcpoint)** returns **text**
    
> Return a JSON version of the data in that point.
>
>     SELECT PC_AsText('010100000064CEFFFF94110000703000000400'::pcpoint);
>
>     {"pcid":1,"pt":[-127,45,124,4]}

**PC_AsBinary(p pcpoint)** returns **bytea**

> Return the OGC "well-known binary" format for the point.
>
>     SELECT PC_AsBinary('010100000064CEFFFF94110000703000000400'::pcpoint);
>
>     \x01010000800000000000c05fc000000000008046400000000000005f40

**PC_Get(pt pcpoint, dimname text)** returns **numeric**

> Return the numeric value of the named dimension. The dimension name
> must exist in the schema.
>
>     SELECT PC_Get('010100000064CEFFFF94110000703000000400'::pcpoint, 'Intensity');
>
>     4

**PC_Patch(pts pcpoint[])** returns **pcpatch**

> Aggregate function that collects a result set of `pcpoint` values into a `pcpatch`.
>
>     INSERT INTO patches (pa)
>     SELECT PC_Patch(pt) FROM points GROUP BY id/10;

**PC_NumPoints(p pcpatch)** returns **integer**

> Return the number of points in this patch.
>
>     SELECT PC_NumPoints(pa) FROM patches LIMIT 1;
> 
>     9     
 
**PC_Envelope(p pcpatch)** returns **bytea**

> Return the OGC "well-known binary" format for *bounds* of the patch.
> Useful for performing intersection tests with geometries.
> 
>     SELECT PC_Envelope(pa) FROM patches LIMIT 1;
>
>     \x0103000000010000000500000090c2f5285cbf5fc0e17a
>     14ae4781464090c2f5285cbf5fc0ec51b81e858b46400ad7
>     a3703dba5fc0ec51b81e858b46400ad7a3703dba5fc0e17a
>     14ae4781464090c2f5285cbf5fc0e17a14ae47814640
 
**PC_AsText(p pcpatch)** returns **text**

> Return a JSON version of the data in that patch.
> 
>     SELECT PC_AsText(pa) FROM patches LIMIT 1;
>
>     {"pcid":1,"pts":[
>      [-126.99,45.01,1,0],[-126.98,45.02,2,0],[-126.97,45.03,3,0],
>      [-126.96,45.04,4,0],[-126.95,45.05,5,0],[-126.94,45.06,6,0],
>      [-126.93,45.07,7,0],[-126.92,45.08,8,0],[-126.91,45.09,9,0]
>     ]}

**PC_Uncompress(p pcpatch)** returns **pcpatch**

> Returns an uncompressed version of the patch (compression type 'none').
> In order to return an uncompressed patch on the wire, this must be the
> outer function with return type `pcpatch` in your SQL query. All 
> other functions that return `pcpatch` will compress output to the
> schema-specified compression before returning.
>
>     SELECT PC_Uncompress(pa) FROM patches 
>        WHERE PC_NumPoints(pa) = 1;
>
>     01010000000000000001000000C8CEFFFFF8110000102700000A00 

**PC_Union(p pcpatch[])** returns **pcpatch**

> Aggregate function merges a result set of `pcpatch` entries into a single `pcpatch`.
>
>     -- Compare npoints(sum(patches)) to sum(npoints(patches))
>     SELECT PC_NumPoints(PC_Union(pa)) FROM patches;
>     SELECT Sum(PC_NumPoints(pa)) FROM patches;
>
>     100 

**PC_Intersects(p1 pcpatch, p2 pcpatch)** returns **boolean**

> Returns true if the bounds of p1 intersect the bounds of p2.
>
>     -- Patch should intersect itself
>     SELECT PC_Intersects(
>              '01010000000000000001000000C8CEFFFFF8110000102700000A00'::pcpatch,
>              '01010000000000000001000000C8CEFFFFF8110000102700000A00'::pcpatch);
>
>     t

**PC_Explode(p pcpatch)** returns **SetOf[pcpoint]**

> Set-returning function, converts patch into result set of one point record for each point in the patch.
>     
>     SELECT PC_AsText(PC_Explode(pa)), id 
>     FROM patches WHERE PC_NumPoints(pa) = 9;
>
>                  pc_astext              | id 
>     -------------------------------------+----
>      {"pcid":1,"pt":[-126.99,45.01,1,0]} |  1
>      {"pcid":1,"pt":[-126.98,45.02,2,0]} |  1
>      {"pcid":1,"pt":[-126.97,45.03,3,0]} |  1
>      {"pcid":1,"pt":[-126.96,45.04,4,0]} |  1
>      {"pcid":1,"pt":[-126.95,45.05,5,0]} |  1
>      {"pcid":1,"pt":[-126.94,45.06,6,0]} |  1
>      {"pcid":1,"pt":[-126.93,45.07,7,0]} |  1
>      {"pcid":1,"pt":[-126.92,45.08,8,0]} |  1
>      {"pcid":1,"pt":[-126.91,45.09,9,0]} |  1



## Binary Formats ##

In order to preserve some compactness in dump files and network transmissions, the binary formats need to retain their native compression.  All binary formats are hex-encoded before output. 

The point and patch binary formats start with a common header, which provides:

- endianness flag, to allow portability between architectures
- pcid number, to look up the schema information in the `pointcloud_formats` table

The patch binary formats have additional standard header information:

- the compression number, which indicates how to interpret the data
- the number of points in the patch

After the header comes the data. Data are packed into the data area following the data types and sizes 


There are two compression schemes currently implemented

- 0, uncompressed, is a simple set of points

), Uncompressed binary is, as advertised, just hex-encoded points and sets of points.

### Point Binary ###

    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uchar[]:  pointdata (interpret relative to pcid)

### Patch Binary ###

    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uint32:       npoints
    uchar[]:  data (interpret relative to pcid and compression)

### Patch Binary (Uncompressed) ####

    byte:         endianness (1 = NDR, 0 = XDR)
    uint32:       pcid (key to POINTCLOUD_SCHEMAS)
    uint32:       0 = no compression
    pointdata[]:  interpret relative to pcid

### Patch Binary (Dimensional) ###

    byte:          endianness (1 = NDR, 0 = XDR)
    uint32:        pcid (key to POINTCLOUD_SCHEMAS)
    uint32:        1 = dimensional compression
    uint32:        npoints
    dimensions[]:  dimensionally compressed data for each dimension

    /* dimensional compression */
    byte:          dimensional compression type (0 = none, 1 = significant bits, 2 = deflate, 3 = run-length)
    
    /* TO DO, more on dimensional compression contents */
