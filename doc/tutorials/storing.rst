******************************************************************************
Storing data
******************************************************************************

.. code-block::

  $ docker pull pgpointcloud/pointcloud

Full documentation: https://hub.docker.com/_/postgres

.. code-block::

  $ wget https://github.com/PDAL/data/raw/master/liblas/LAS12_Sample_withRGB_Quick_Terrain_Modeler_fixed.laz
  $ pdal info LAS12_Sample_withRGB_Quick_Terrain_Modeler_fixed.laz --summary

3811489 points
EPSG 32616

.. code-block::

  $ docker run --name pgpointcloud -e POSTGRES_DBNAME=airport -e POSTGRES_PASSWORD=mysecretpassword -d pgpointcloud/pointcloud
  $ docker exec -it pgpointcloud psql -U postgres -d pointclouds -c "\dx"
                                            List of installed extensions
          Name          | Version |   Schema   |                             Description
  ------------------------+---------+------------+---------------------------------------------------------------------
   fuzzystrmatch          | 1.1     | public     | determine similarities and distance between strings
   plpgsql                | 1.0     | pg_catalog | PL/pgSQL procedural language
   pointcloud             | 1.2.1   | public     | data type for lidar point clouds
   pointcloud_postgis     | 1.2.1   | public     | integration for pointcloud LIDAR data and PostGIS geometry data
   postgis                | 3.0.1   | public     | PostGIS geometry, geography, and raster spatial types and functions
   postgis_tiger_geocoder | 3.0.1   | tiger      | PostGIS tiger geocoder and reverse geocoder
   postgis_topology       | 3.0.1   | topology   | PostGIS topology spatial types and functions
  (7 rows)
  $ docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' pgpointcloud
  172.17.0.2

.. code-block::

  {
  "pipeline":[
    {
      "type":"readers.las",
      "filename":"/home/pblottiere/tmp/pgpointcloud_tuto/LAS12_Sample_withRGB_Quick_Terrain_Modeler_fixed.laz"
    },
    {
      "type":"filters.chipper",
      "capacity":"400"
    },
    {
      "type":"writers.pgpointcloud",
      "connection":"host='172.17.0.2' dbname='pointclouds' user='postgres' password='mysecretpassword' port='5432'",
      "table":"airport",
      "compression":"none"
    }
  ]
}

.. code-block::

  $ pdal pipeline pipeline.json

.. code-block::

  [pgpointcloud]
  host=172.17.0.2
  port=5432
  dbname=pointclouds
  user=postgres
  password=mysecretpassword

.. code-block::

  $ psql service=pgpointcloud
  psql (12.3)
  Type "help" for help.

  pointclouds=# select count(*) from airport;
   count
  -------
    9529
  (1 row)

