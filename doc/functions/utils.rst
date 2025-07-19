.. _utils:

********************************************************************************
Utils
********************************************************************************

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_Full_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_Full_Version() returns text:

Return a composite version string summarizing pointcloud system components.
Includes library, SQL, libxml2, PostgreSQL interface versions, and LAZperf
support flag.  Useful for debugging or verifying runtime compatibility of the
pointcloud extension.

.. code-block::

    SELECT PC_Full_Version();

    POINTCLOUD="1.2.5 2346cc2" PGSQL="170" LIBXML2="2.14.3 LAZPERF enabled=false

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_Lazperf_Enabled
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_Lazperf_Enabled() returns boolean:

Return `true` if the pointcloud extension includes LAZperf compression support.

.. code-block::

    SELECT PC_Lazperf_Enabled();

    t

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_LibXML2_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_LibXML2_Version() returns text:

Return the `libxml2` version number.

.. code-block::

    SELECT PC_LibXML2_Version();

    2.14.3

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_Lib_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_Lib_Version() returns text:

Return the library version number.

.. code-block::

    SELECT PC_Lib_Version();

    1.2.5 2346cc2

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_PGSQL_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_PGSQL_Version() returns text:

Return the `pgsql` version number.

.. code-block::

    SELECT PC_PGSQL_Version();

    170

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_PostGIS_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_PostGIS_Version() returns text:

Return the PostGIS extension version number.

.. code-block::

    SELECT PC_PostGIS_Version();

    1.2.5

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_Script_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_Script_Version() returns text:

Return the script version number.

.. code-block::

    SELECT PC_Script_Version();

    1.2.5

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_Version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_Version() returns text:

Return the extension version number.

.. code-block::

    SELECT PC_Version();

    1.2.5
