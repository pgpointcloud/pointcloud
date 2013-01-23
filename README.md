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

Uncompressed Binary
-------------------

    PCPOINT
    
    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uchar[]:  data (interpret relative to pcid)


    PCPATCH

    byte:     endianness (1 = NDR, 0 = XDR)
    uint32:   pcid (key to POINTCLOUD_SCHEMAS)
    uint32:   npoints
    uchar[]:  data (interpret relative to pcid)
