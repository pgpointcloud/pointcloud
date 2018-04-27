[![Build Status](https://travis-ci.org/pgpointcloud/pointcloud.svg?branch=master)](https://travis-ci.org/pgpointcloud/pointcloud)

# Pointcloud #

A PostgreSQL extension for storing point cloud (LIDAR) data.

- Mailing list: http://lists.osgeo.org/mailman/listinfo/pgpointcloud/


## Build/Install ##

### Requirements ###

- PostgreSQL and PostgreSQL development packages must be installed (pg_config and server headers). For Red Hat and Ubuntu, the package names are usually "postgresql-dev" or "postgresql-devel"
- LibXML2 development packages must be installed, usually "libxml2-dev" or "libxml2-devel".
- CUnit packages must be installed, or [source built and installed](http://sourceforge.net/projects/cunit/ "CUnit").
- [Optional] GHT library may be installed for GHT compression support, [built from source](http://github.com/pramsey/libght/ "LibGHT")
- [Optional] LAZPERF library may be installed for LAZ compression support, [built from source](http://github.com/hobu/laz-perf "LAZPERF")

### Build ###

After generating the configure script with ``autogen.sh``,  ``./configure --help`` to get a complete listing of configuration options.

- ``./autogen.sh``
- ``./configure``
- ``make``
- ``sudo make install``

Note: you can use ``--with-pgconfig`` on the ``./configure`` command line if you have multiple PostgreSQL installations on your system and want to target a specific one. For example:

- ``./configure --with-pgconfig=/sur/lib/postgresql/9.5/bin/pg_config``

Run unit tests:

- ``make check``

Note that if you configured PointCloud using a non-standard LibGHT location, you may need to add its location to the ``LD_LIBRARY_PATH`` environment variable. For example:

- ``LD_LIBRARY_PATH=$HOME/local/lib make check``

### SQL Tests ###

pointcloud includes SQL tests to run against an existing installation.

Run the SQL tests:

- `sudo make install`
- `PGUSER=a_user PGPASSWORD=a_password PGHOST=localhost make installcheck`

This command will create a database named `contrib_regression` and will execute the SQL scripts located in `pgsql/sql` in this database.

### Activate ###

- Create a new database: ``CREATE DATABASE mynewdb``
- Connect to that database.
- Activate the pointcloud extension: ``CREATE EXTENSION pointcloud``

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
      </pc:metadata>
    </pc:PointCloudSchema>');

Schema documents are stored in the ``pointcloud_formats`` table, along with a ``pcid`` or "pointcloud identifier". Rather than store the whole schema information with each database object, each object just has a `pcid`, which serves as a key to find the schema in ``pointcloud_formats``.  This is similar to the way the ``srid`` is resolved for spatial reference system support in [PostGIS](http://postgis.net).

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

### PcPoint Functions

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

**PC_PCId(p pcpoint)** returns **integer** (from 1.1.0)

> Return the `pcid` schema number of this point.
>
>     SELECT PC_PCId('010100000064CEFFFF94110000703000000400'::pcpoint);
> 
>     1     

**PC_Get(pt pcpoint, dimname text)** returns **numeric**

> Return the numeric value of the named dimension. The dimension name
> must exist in the schema.
>
>     SELECT PC_Get('010100000064CEFFFF94110000703000000400'::pcpoint, 'Intensity');
>
>     4

**PC_Get(pt pcpoint)** returns **float8[]**  (from 1.1.0)

> Return values of all dimensions in an array.
>
>     SELECT PC_Get('010100000064CEFFFF94110000703000000400'::pcpoint);
>
>     {-127,45,124,4}

### PcPatch Functions

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

**PC_PCId(p pcpatch)** returns **integer** (from 1.1.0)

> Return the `pcid` schema number of points in this patch.
>
>     SELECT PC_PCId(pa) FROM patches LIMIT 1;
> 
>     1     

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

**PC_Summary(p pcpatch)** returns **text** (from 1.1.0)

> Return a JSON formatted summary of the data in that point.
>
>     SELECT PC_Summary(pa) FROM patches LIMIT 1;
>
>     {"pcid":1, "npts":9, "srid":4326, "compr":"dimensional","dims":[{"pos":0,"name":"X","size":4,"type":"int32_t","compr":"sigbits","stats":{"min":-126.99,"max":-126.91,"avg":-126.95}},{"pos":1,"name":"Y","size":4,"type":"int32_t","compr":"sigbits","stats":{"min":45.01,"max":45.09,"avg":45.05}},{"pos":2,"name":"Z","size":4,"type":"int32_t","compr":"sigbits","stats":{"min":1,"max":9,"avg":5}},{"pos":3,"name":"Intensity","size":2,"type":"uint16_t","compr":"rle","stats":{"min":0,"max":0,"avg":0}}]}

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
>     FROM patches WHERE id = 7;
>
>                   pc_astext               | id 
>     --------------------------------------+----
>      {"pcid":1,"pt":[-126.5,45.5,50,5]}   |  7
>      {"pcid":1,"pt":[-126.49,45.51,51,5]} |  7
>      {"pcid":1,"pt":[-126.48,45.52,52,5]} |  7
>      {"pcid":1,"pt":[-126.47,45.53,53,5]} |  7
>      {"pcid":1,"pt":[-126.46,45.54,54,5]} |  7
>      {"pcid":1,"pt":[-126.45,45.55,55,5]} |  7
>      {"pcid":1,"pt":[-126.44,45.56,56,5]} |  7
>      {"pcid":1,"pt":[-126.43,45.57,57,5]} |  7
>      {"pcid":1,"pt":[-126.42,45.58,58,5]} |  7
>      {"pcid":1,"pt":[-126.41,45.59,59,5]} |  7

**PC_PatchAvg(p pcpatch, dimname text)** returns **numeric**

> Reads the values of the requested dimension for all points in the patch 
> and returns the *average* of those values. Dimension name must exist in the schema.
>
>     SELECT PC_PatchAvg(pa, 'intensity') 
>     FROM patches WHERE id = 7;
>
>     5.0000000000000000

**PC_PatchMax(p pcpatch, dimname text)** returns **numeric**

> Reads the values of the requested dimension for all points in the patch 
> and returns the *maximum* of those values. Dimension name must exist in the schema.
>
>     SELECT PC_PatchMax(pa, 'x') 
>     FROM patches WHERE id = 7;
>
>     -126.41

**PC_PatchMin(p pcpatch, dimname text)** returns **numeric**

> Reads the values of the requested dimension for all points in the patch 
> and returns the *minimum* of those values. Dimension name must exist in the schema.
>
>     SELECT PC_PatchMin(pa, 'y') 
>     FROM patches WHERE id = 7;
>
>     45.5

**PC_PatchAvg(p pcpatch)** returns **pcpoint** (from 1.1.0)

> Returns a PcPoint with the *average* values of each dimension in the patch.
>
>     SELECT PC_AsText(PC_PatchAvg(pa))
>     FROM patches WHERE id = 7;
>
>     {"pcid":1,"pt":[-126.46,45.54,54.5,5]}

**PC_PatchMax(p pcpatch)** returns **pcpoint** (from 1.1.0)

> Returns a PcPoint with the *maximum* values of each dimension in the patch.
>
>     SELECT PC_PatchMax(pa)
>     FROM patches WHERE id = 7;
>
>     {"pcid":1,"pt":[-126.41,45.59,59,5]}

**PC_PatchMin(p pcpatch)** returns **pcpoint** (from 1.1.0)

> Returns a PcPoint with the *minimum* values of each dimension in the patch.
>
>     SELECT PC_PatchMin(pa)
>     FROM patches WHERE id = 7;
>
>     {"pcid":1,"pt":[-126.5,45.5,50,5]}

**PC_FilterGreaterThan(p pcpatch, dimname text, float8 value)** returns **pcpatch**

> Returns a patch with only points whose values are greater than the supplied value
> for the requested dimension.
>
>     SELECT PC_AsText(PC_FilterGreaterThan(pa, 'y', 45.57)) 
>     FROM patches WHERE id = 7;
>
>      {"pcid":1,"pts":[[-126.42,45.58,58,5],[-126.41,45.59,59,5]]}

**PC_FilterLessThan(p pcpatch, dimname text, float8 value)** returns **pcpatch**

> Returns a patch with only points whose values are less than the supplied value
> for the requested dimension.

**PC_FilterBetween(p pcpatch, dimname text, float8 value1, float8 value2)** returns **pcpatch**

> Returns a patch with only points whose values are between the supplied values
> for the requested dimension.

**PC_FilterEquals(p pcpatch, dimname text, float8 value)** returns **pcpatch**

> Returns a patch with only points whose values are the same as the supplied values
> for the requested dimension.

**PC_Compress(p pcpatch,global_compression_scheme text,compression_config text)** returns **pcpatch** (from 1.1.0)

> Compress a patch with a manually specified scheme.
> The compression_config semantic depends on the global compression scheme.
> Allowed global compression schemes are:
>  - auto -- determined by pcid
>  - ght  -- no compression config supported
>  - laz -- no compression config supported
>  - dimensional
>      configuration is a comma-separated list of per-dimension
>      compressions from this list:
>      - auto -- determined automatically, from values stats
>      - zlib -- deflate compression
>      - sigbits -- significant bits removal
>      - rle -- run-length encoding

**PC_PointN(p pcpatch, n int4)** returns **pcpoint**

> Returns the n-th point of the patch with 1-based indexing. Negative n counts point from the end. 

**PC_IsSorted(p pcpatch, dimnames text[], strict boolean default true)** returns **boolean**

> Checks whether a pcpatch is sorted lexicographically along the given dimensions. The `strict` option further checks that the ordering is strict (no duplicates).

**PC_Sort(p pcpatch, dimnames text[])** returns **pcpatch**

> Returns a copy of the input patch lexicographically sorted along the given dimensions.

**PC_Range(p pcpatch, start int4, n int4)** returns **pcpatch**

> Returns a patch containing *n* points. These points are selected from the *start*-th point with 1-based indexing.

**PC_SetPCId(p pcpatch, pcid int4, def float8 default 0.0)** returns **pcpatch**

> Sets the schema on a PcPatch, given a valid `pcid` schema number.
>
> For dimensions that are in the "new" schema but not in the "old" schema the value `def` is set in the points of the output patch. `def` is optional, its default value is `0.0`.

**PC_Transform(p pcpatch, pcid int4, def float8 default 0.0)** returns **pcpatch**

> Returns a new patch with its data transformed based on the schema whose identifier is `pcid`.
>
> For dimensions that are in the "new" schema but not in the "old" schema the value `def` is set in the points of the output patch. `def` is optional, its default value is `0.0`.
>
> Contrary to `PC_SetPCId`, `PC_Transform` may change (transform) the patch data if dimension interpretations, scales or offsets are different in the new schema.

### OGC "well-known binary" Functions

**PC_AsBinary(p pcpoint)** returns **bytea**

> Return the OGC "well-known binary" format for the point.
>
>     SELECT PC_AsBinary('010100000064CEFFFF94110000703000000400'::pcpoint);
>
>     \x01010000800000000000c05fc000000000008046400000000000005f40

**PC_EnvelopeAsBinary(p pcpatch)** returns **bytea**

> Return the OGC "well-known binary" format for the 2D *bounds* of the patch.
> Useful for performing 2D intersection tests with geometries.
>
>     SELECT PC_EnvelopeAsBinary(pa) FROM patches LIMIT 1;
>
>     \x0103000000010000000500000090c2f5285cbf5fc0e17a
>     14ae4781464090c2f5285cbf5fc0ec51b81e858b46400ad7
>     a3703dba5fc0ec51b81e858b46400ad7a3703dba5fc0e17a
>     14ae4781464090c2f5285cbf5fc0e17a14ae47814640
>
> **PC_Envelope** is an alias to **PC_EnvelopeAsBinary**. But **PC_Envelope** is deprecated and will be removed in a future version (2.0) of the extension. **PC_EnvelopeAsBinary** is to be used instead.

**PC_BoundingDiagonalAsBinary(p pcpatch)** returns **bytea**

> Return the OGC "well-known binary" format for the bounding diagonal of the patch.
>
>    SELECT PC_BoundingDiagonalAsBinary(
>        PC_Patch(ARRAY[
>            PC_MakePoint(1, ARRAY[0.,0.,0.,10.]),
>            PC_MakePoint(1, ARRAY[1.,1.,1.,10.]),
>            PC_MakePoint(1, ARRAY[10.,10.,10.,10.])]));
>
>    \x01020000a0e610000002000000000000000000000000000000000000000000000000000000000000000000244000000000000024400000000000002440

## PostGIS Integration ##

The `pointcloud_postgis` extension adds functions that allow you to use PostgreSQL Pointcloud with PostGIS, converting PcPoint and PcPatch to Geometry and doing spatial filtering on point cloud data. The `pointcloud_postgis` extension depends on both the `postgis` and `pointcloud` extensions, so they must be installed first:

    CREATE EXTENSION postgis;
    CREATE EXTENSION pointcloud;
    CREATE EXTENSION pointcloud_postgis;
    
**PC_Intersects(p pcpatch, g geometry)** returns **boolean**<br/>
**PC_Intersects(g geometry, p pcpatch)** returns **boolean**

> Returns true if the bounds of the patch intersect the geometry.
>
>     SELECT PC_Intersects('SRID=4326;POINT(-126.451 45.552)'::geometry, pa)
>     FROM patches WHERE id = 7;
>
>     t

**PC_Intersection(pcpatch, geometry)** returns **pcpatch**

> Returns a PcPatch which only contains points that intersected the 
> geometry.
>
>     SELECT PC_AsText(PC_Explode(PC_Intersection(
>           pa, 
>           'SRID=4326;POLYGON((-126.451 45.552, -126.42 47.55, -126.40 45.552, -126.451 45.552))'::geometry
>     )))
>     FROM patches WHERE id = 7;
>
>                  pc_astext               
>     --------------------------------------
>      {"pcid":1,"pt":[-126.44,45.56,56,5]}
>      {"pcid":1,"pt":[-126.43,45.57,57,5]}
>      {"pcid":1,"pt":[-126.42,45.58,58,5]}
>      {"pcid":1,"pt":[-126.41,45.59,59,5]}

**Geometry(pcpoint)** returns **geometry**<br/>
**pcpoint::geometry** returns **geometry**

> Casts PcPoint to the PostGIS geometry equivalent, placing the x/y/z/m of the
> PcPoint into the x/y/z/m of the PostGIS point.
> 
>     SELECT ST_AsText(PC_MakePoint(1, ARRAY[-127, 45, 124.0, 4.0])::geometry);
> 
>     POINT Z (-127 45 124)

**PC_EnvelopeGeometry(pcpatch)** returns **geometry**

> Returns the 2D *bounds* of the patch as a PostGIS Polygon 2D.
> Useful for performing 2D intersection tests with PostGIS geometries.
>
>     SELECT ST_AsText(PC_EnvelopeGeometry(pa)) FROM patches LIMIT 1;
>
>     POLYGON((-126.99 45.01,-126.99 45.09,-126.91 45.09,-126.91 45.01,-126.99 45.01))

**PC_BoundingDiagonalGeometry(pcpatch)** returns **geometry**

> Returns the bounding diagonal of a patch. This is a LineString (2D), a LineString Z or a LineString M or a LineString ZM, based on the existence of the Z and M dimensions in the patch. This function is useful for creating an index on a patch column.
>
>     SELECT ST_AsText(PC_BoundingDiagonalGeometry(pa)) FROM patches;
>                       st_astext
>     ------------------------------------------------
>     LINESTRING Z (-126.99 45.01 1,-126.91 45.09 9)
>     LINESTRING Z (-126 46 100,-126 46 100)
>     LINESTRING Z (-126.2 45.8 80,-126.11 45.89 89)
>     LINESTRING Z (-126.4 45.6 60,-126.31 45.69 69)
>     LINESTRING Z (-126.3 45.7 70,-126.21 45.79 79)
>     LINESTRING Z (-126.8 45.2 20,-126.71 45.29 29)
>     LINESTRING Z (-126.5 45.5 50,-126.41 45.59 59)
>     LINESTRING Z (-126.6 45.4 40,-126.51 45.49 49)
>     LINESTRING Z (-126.9 45.1 10,-126.81 45.19 19)
>     LINESTRING Z (-126.7 45.3 30,-126.61 45.39 39)
>     LINESTRING Z (-126.1 45.9 90,-126.01 45.99 99)
>
> For example, this is how one may want to create an index:
>
>     CREATE INDEX ON patches USING GIST(PC_BoundingDiagonalGeometry(patch) gist_geometry_ops_nd);

## Compressions ##

One of the issues with LIDAR data is that there is a lot of it. To deal with data volumes, PostgreSQL Pointcloud allows schemas to declare their preferred compression method in the `<pc:metadata>` block of the schema document. In the example schema, we declared our compression as follows:

    <pc:metadata>
      <Metadata name="compression">dimensional</Metadata>
    </pc:metadata>

There are currently four supported compressions:

- **None**, which stores points and patches as byte arrays using the type and formats described in the schema document.
- **Dimensional**, which stores points the same as 'none' but stores patches as collections of dimensional data arrays, with an "appropriate" compression applied. Dimensional compression makes the most sense for smaller patch sizes, since small patches will tend to have more homogeneous dimensions.
- **GHT** or "GeoHash Tree", which stores the points in a tree where each node stores the common values shared by all nodes below. For larger patch sizes, GHT should provide effective compression and performance for patch-wise operations. You must build Pointcloud with libght support to make use of the GHT compression.
- **LAZ** or "LASZip". You must build Pointcloud with LAZPERF support to make use of the LAZ compression.

If no compression is declared in `<pc:metadata>`, then a compression of "none" is assumed.

### Dimensional Compression ###

Dimensional compression first flips the patch representation from a list of N points containing M dimension values to a list of M dimensions each containing N values.

    {"pcid":1,"pts":[
          [-126.99,45.01,1,0],[-126.98,45.02,2,0],[-126.97,45.03,3,0],
          [-126.96,45.04,4,0],[-126.95,45.05,5,0],[-126.94,45.06,6,0]
         ]}

Becomes, notionally:

    {"pcid":1,"dims":[
          [-126.99,-126.98,-126.97,-126.96,-126.95,-126.94],
          [45.01,45.02,45.03,45.04,45.05,45.06],
          [1,2,3,4,5,6],
          [0,0,0,0,0,0]
         ]}

The potential benefit for compression is that each dimension has quite different distribution characteristics, and is amenable to different approaches.  In this example, the fourth dimension (intensity) can be very highly compressed with run-length encoding (one run of six zeros). The first and second dimensions have relatively low variability relative to their magnitude and can be compressed by removing the repeated bits.

Dimensional compression currently uses only three compression schemes:

- run-length encoding, for dimensions with low variability
- common bits removal, for dimensions with variability in a narrow bit range
- raw deflate compression using zlib, for dimensions that aren't amenable to the other schemes

For LIDAR data organized into patches of points that sample similar areas, the dimensional scheme compresses at between 3:1 and 5:1 efficiency.


## Binary Formats ##

In order to preserve some compactness in dump files and network transmissions, the binary formats need to retain their native compression.  All binary formats are hex-encoded before output. 

The point and patch binary formats start with a common header, which provides:

- endianness flag, to allow portability between architectures
- pcid number, to look up the schema information in the `pointcloud_formats` table

The patch binary formats have additional standard header information:

- the compression number, which indicates how to interpret the data
- the number of points in the patch


### Point Binary ###

    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uchar[]:  pointdata (interpret relative to pcid)

### Patch Binary ###

### Patch Binary (Uncompressed) ####

    byte:         endianness (1 = NDR, 0 = XDR)
    uint32:       pcid (key to POINTCLOUD_SCHEMAS)
    uint32:       0 = no compression
    uint32:       npoints
    pointdata[]:  interpret relative to pcid

### Patch Binary (Dimensional) ###

    byte:          endianness (1 = NDR, 0 = XDR)
    uint32:        pcid (key to POINTCLOUD_SCHEMAS)
    uint32:        2 = dimensional compression
    uint32:        npoints
    dimensions[]:  dimensionally compressed data for each dimension

Each compressed dimension starts with a byte, that gives the compression type, and then a uint32 that gives the size of the segment in bytes.

    byte:           dimensional compression type (0-3)
    uint32:         size of the compressed dimension in bytes
    data[]:         the compressed dimensional values

There are four possible compression types used in dimensional compression:

- no compression = 0,
- run-length compression = 1,
- significant bits removal = 2,
- deflate = 3

    
#### No dimension compress ####

For dimensional compression 0 (no compression) the values just appear in order. The length of words in this dimension must be determined from the schema document.

    word[]:

#### Run-length compress dimension ####

For run-length compression, the data stream consists of a set of pairs: a byte value indicating the length of the run, and a data value indicating the value that is repeated.

     byte:          number of times the word repeats
     word:          value of the word being repeated
     ....           repeated for the number of runs

The length of words in this dimension must be determined from the schema document.

#### Significant bits removal on dimension ####

Significant bits removal starts with two words. The first word just gives the number of bits that are "significant", that is the number of bits left after the common bits are removed from any given word. The second word is a bitmask of the common bits, with the final, variable bits zeroed out.

     word1:          number of variable bits in this dimension
     word2:          the bits that are shared by every word in this dimension
     data[]:         variable bits packed into a data buffer

#### Deflate dimension ####

Where simple compression schemes fail, general purpose compression is applied to the dimension using zlib. The data area is a raw zlib buffer suitable for passing directly to the inflate() function. The size of the input buffer is given in the common dimension header. The size of the output buffer can be derived from the patch metadata by multiplying the dimension word size by the number of points in the patch.

### Patch Binary (GHT) ####

    byte:          endianness (1 = NDR, 0 = XDR)
    uint32:        pcid (key to POINTCLOUD_SCHEMAS)
    uint32:        1 = GHT compression
    uint32:        npoints
    uint32:        GHT data size
    uint8:         GHT data

GHT patches are much like dimensional patches, except their internal structure is more opaque. Use LibGHT to read the GHT data buffer out into a GHT tree in memory.

### Patch Binary (LAZ) ####

    byte:          endianness (1 = NDR, 0 = XDR)
    uint32:        pcid (key to POINTCLOUD_SCHEMAS)
    uint32:        3 = LAZ compression
    uint32:        npoints
    uint32:        LAZ data size
    data[]:        LAZ data

LAZ patches are much like GHT patches. Use LAZPERF library to read the LAZ data buffer out into a LAZ buffer.

## Loading Data ##

The examples above show how to form patches from array of doubles, and well-known binary. You can write your own loader, using the uncompressed WKB format, or more simply you can load existing LIDAR files using the [PDAL](https://www.pdal.io) processing and format conversion library.

### From WKB ###

If you are writing your own loading system and want to write into Pointcloud types, create well-known binary inputs, in uncompressed format. If you schema indicates that your patch storage is compressed, Pointcloud will automatically compress your patch before storing it, so you can create patches in uncompressed WKB without worrying about the nuances of particular internal compression schemes.

The only issues to watch when creating WKB patches are: ensuring the data you write is sized according to the schema (use the specified dimension type); ensuring that the endianness of the data matches the declared endianness of the patch.

### From PDAL ###

#### Build and Install PDAL ####

To build and install PDAL check out the [PDAL development
documentation](https://www.pdal.io/development).

With PDAL installed you're ready to run a PDAL import into PostgreSQL PointCloud!

#### Running `pdal pipeline` ####

PDAL includes a [command line program](http://www.pointcloud.org/apps.html) that allows both simple format translations and more complex "pipelines" of transformation.  The `pdal translate` does simple format transformations. In order to load data into Pointcloud we use a "PDAL pipeline", by calling `pdal pipeline`. A pipeline combines a format reader, and format writer, with filters that can alter or group the points together.

PDAL pipelines are JSON files, which declare readers, filters, and writers forming a processing chain that will be applied to the LIDAR data.

To execute a pipeline file, run it through the `pdal pipeline` command:

    pdal pipeline --input pipelinefile.json

Here is a simple example pipeline that reads a LAS file and writes into a PostgreSQL Pointcloud database.

```json
{
  "pipeline":[
    {
      "type":"readers.las",
      "filename":"/home/lidar/st-helens-small.las",
      "spatialreference":"EPSG:26910"
    },
    {
      "type":"filters.chipper",
      "capacity":400
    },
    {
      "type":"writers.pgpointcloud",
      "connection":"host='localhost' dbname='pc' user='lidar' password='lidar' port='5432'",
      "table":"sthsm",
      "compression":"dimensional",
      "srid":"26910"
    }
  ]
}
```

PostgreSQL Pointcloud storage of LIDAR works best when each "patch" of points consists of points that are close together, and when most patches do not overlap. In order to convert unordered data from a LIDAR file into patch-organized data in the database, we need to pass it through a filter to "chip" the data into compact patches. The "chipper" is one of the filters we need to apply to the data while loading.

Similarly, reading data from a PostgreSQL Pointcloud uses a Pointcloud reader and a file writer of some sort. This example reads from the database and writes to a CSV text file:

```json
{
  "pipeline":[
    {
      "type":"readers.pgpointcloud",
      "connection":"host='localhost' dbname='pc' user='lidar' password='lidar' port='5432'",
      "table":"sthsm",
      "column":"pa",
      "spatialreference":"EPSG:26910"
    },
    {
      "type":"writers.text",
      "filename":"/home/lidar/st-helens-small-out.txt"
    }
  ]
}
```

Note that we do not need to chip the data stream when reading from the database, as the writer does not care if the points are blocked into patches or not.

You can use the "where" option to restrict a read to just an envelope, allowing partial extracts from a table:

```json
{
  "pipeline":[
    {
      "type":"readers.pgpointcloud",
      "connection":"host='localhost' dbname='pc' user='lidar' password='lidar' port='5432'",
      "table":"sthsm",
      "column":"pa",
      "spatialreference":"EPSG:26910",
      "where":"PC_Intersects(pa, ST_MakeEnvelope(560037.36, 5114846.45, 562667.31, 5118943.24, 26910))",
    },
    {
      "type":"writers.text",
      "filename":"/home/lidar/st-helens-small-out.txt"
    }
  ]
}
```

#### PDAL pgpointcloud Reader/Writer Options ####

The PDAL [writers.pgpointcloud](http://www.pdal.io/stages/writers.pgpointcloud.html) for PostgreSQL Pointcloud takes the following options:

- **connection**: The PostgreSQL database connection string. E.g. `host=localhost user=username password=pw db=dbname port=5432`
- **table**: The database table create to write the patches to.
- **schema**: The schema to create the table in. [Optional]
- **column**: The column name to use in the patch table. [Optional: "pa"]
- **compression**: The patch compression format to use [Optional: "dimensional"]
- **overwrite**: Replace any existing table [Optional: true]
- **srid**: The spatial reference id to store data in [Optional: 4326]
- **pcid**: An existing PCID to use for the point cloud schema [Optional]
- **pre_sql**: Before the pipeline runs, read and execute this SQL file or command [Optional]
- **post_sql**: After the pipeline runs, read and execute this SQL file or command [Optional]
 
The PDAL [readers.pgpointcloud](http://www.pdal.io/stages/readers.pgpointcloud.html) for PostgreSQL Pointcloud takes the following options:

- **connection**: The PostgreSQL database connection string. E.g. `host=localhost user=username password=pw db=dbname port=5432`
- **table**: The database table to read the patches from.
- **schema**: The schema to read the table from. [Optional] 
- **column**: The column name in the patch table to read from. [Optional: "pa"]
- **where**: SQL where clause to constrain the query [Optional]
- **spatialreference**: Overrides the database declared SRID [Optional]

