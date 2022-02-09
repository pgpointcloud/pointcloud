******************************************************************************
Schema and compression
******************************************************************************

This tutorial is an introduction for investigating XML schemas and playing with
compression of patches.

------------------------------------------------------------------------------
Compression type
------------------------------------------------------------------------------

The compression of a patch may be retrieved through its XML schema but it's
also stored in the patch itself. Of course, both needs to be consistent so
updating an existing schema is hardly discouraged and may lead to errors.

In the first case, the XML schema needs to be parsed with ``xpath`` function to
retrieve the ``pc:metadata`` tag of a specific patch:

.. code-block:: sql

  pointclouds=# WITH tmp AS (
                    SELECT pc_pcid(pa)
                    AS _pcid
                    FROM airport
                    LIMIT 1
                )
                SELECT unnest(
                    xpath(
                        '/pc:PointCloudSchema/pc:metadata/Metadata/text()',
                        schema::xml,
                        array[
                            ['pc', 'http://pointcloud.org/schemas/PC/'],
                            ['xsi', 'http://www.w3.org/2001/XMLSchema-instance']
                        ]
                    )
                )
                AS "compression"
                FROM tmp,pointcloud_formats
                WHERE pcid=tmp._pcid;

     metadata
  ---------------
    dimensional
  (1 row)


A much easier way to retrieve the compression type is to take a look to the
JSON summary of the patch:

.. code-block:: sql

  pointclouds=# SELECT pc_summary(pa)::json->'compr'
                AS "compression"
                FROM airport
                LIMIT 1;

    compression
  ---------------
   "dimensional"
  (1 row)


.. code-block:: sql

  pointclouds=# INSERT INTO pointcloud_formats (pcid, srid, schema)
                VALUES (
                    34,
                    4326,
                    (
                      SELECT Regexp_replace(schema, 'dimensional', 'none', 'g')
                      FROM pointcloud_formats
                      WHERE  pcid = 3
                    )
                ); 



$ create table pa_dim_to_none as select pc_transform(pa, 35) as pa from pa_dim;


------------------------------------------------------------------------------
Update compression
------------------------------------------------------------------------------
