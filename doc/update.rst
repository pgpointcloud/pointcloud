.. _update:

******************************************************************************
Update
******************************************************************************

pgPointcloud extension
------------------------------------------------------------------------------

Once a new version of pgPointcloud installed, you may want to update your
databases where the extension is already in use. The first thing to compare is
the version currently used with versions actually available on your system:

.. code-block:: bash

   mydatabase=# SELECT pc_version();
    pc_version
   ------------
    1.1.1
   (1 row)

   mydatabase=# SELECT version FROM pg_available_extension_versions WHERE name ='pointcloud';
     version
   -----------
    1.1.1
    1.2.1
   (2 rows)


Then you can update to the latest version with ``ALTER EXTENSION pointcloud
UPDATE`` or target a specific version:

.. code-block:: bash

   mydatabase=# ALTER EXTENSION pointcloud UPDATE TO '1.2.1';
   ALTER EXTENSION
   mydatabase=# SELECT pc_version();
    pc_version
   ------------
    1.2.1
   (1 row)


.. warning::

   The GHT compression has been removed in the 1.2.0 version. Unfortunately,
   you have to remove the compression before updating the extension from 1.1.x
   to a higher version.
