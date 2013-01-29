Point Cloud
===========

A PostgreSQL extension for storing point cloud (LIDAR) data.

Requires
========

- PostgreSQL 9.1+ (support for extensions)
- PostgreSQL compiled --with-xml


Build/Install
=============

- Requires PostgreSQL dev packages (pg_config and server headers), usually 'postgresql-dev' or 'postgresql-devel'
- Edit config.mk and update paths to libxml2 and cunit
- make
- sudo make install
- create a database
- run 'CREATE EXTENSION pointcloud'

Formats
=======

In order to preserve some compactness in dump files and network transmissions, the binary formats need to retain their native compression. The human readable format is mostly for debugging and tests, and is not compressed. Uncompressed binary is, as advertised, just hex-encoded points and sets of points.

Human-Readable Text
-------------------

    ( <pcid> : <dim1>, <dim2>, <dim3>, <dim4> )

    [ <pcid> : ( <dim1>, <dim2>, <dim3>, <dim4> ), 
               ( <dim1>, <dim2>, <dim3>, <dim4> ), 
               ( <dim1>, <dim2>, <dim3>, <dim4> ) ]

Point Binary
------------

    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uchar[]:  pointdata (interpret relative to pcid)

Patch Binary
------------

    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   compression (0 = no compression, 1 = dimensional, 2 = GHT)
    uchar[]:  data (interpret relative to pcid and compression)

Patch Binary (Uncompressed)
---------------------------

    byte:         endianness (1 = NDR, 0 = XDR)
    uint32:       pcid (key to POINTCLOUD_SCHEMAS)
    uint32:       0 = no compression
    uint32:       npoints
    pointdata[]:  interpret relative to pcid

Patch Binary (Dimensional)
--------------------------

    byte:          endianness (1 = NDR, 0 = XDR)
    uint32:        pcid (key to POINTCLOUD_SCHEMAS)
    uint32:        1 = dimensional compression
    dimensional[]: dimensionally compressed data for each dimension

    /* dimensional compression */
    byte:          dimensional compression type (0 = none, 1 = significant bits, 2 = deflate, 3 = run-length)
    
    /* TO DO, more on dimensional compression contents */
