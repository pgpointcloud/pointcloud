Point Cloud
===========

A PostgreSQL extension for storing point cloud (LIDAR) data.

Requires
========

- PostgreSQL 9.1+ (support for extensions)
- PostgreSQL compiled --with-xml

Formats
=======

In order to preserve some compactness in dump files and network transmissions, the binary formats need to retain their native compression. The human readable format is mostly for debugging and tests, and is not compressed. Uncompressed binary is, as advertised, just hex-encoded points and sets of points.

Human-Readable Text
-------------------

    PCPOINT( <pcid> : <dim1>, <dim2>, <dim3>, <dim4> )

    PCPATCH( <pcid> : ( <dim1>, <dim2>, <dim3>, <dim4> ), 
                      ( <dim1>, <dim2>, <dim3>, <dim4> ), 
                      ( <dim1>, <dim2>, <dim3>, <dim4> ) 
           )
         
Uncompressed Binary
-------------------

