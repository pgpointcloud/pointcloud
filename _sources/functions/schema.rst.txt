.. _schema:

********************************************************************************
Schema
********************************************************************************

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_SchemaGetNDims
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_SchemaGetNDims(pcid integer) returns integer:

Return the number of dimensions in the corresponding schema.

.. code-block::

    SELECT PC_SchemaGetNDims(1);

    18

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PC_SchemaIsValid
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:PC_SchemaIsValid(xml text) returns boolean:

Return `true` if the pointcloud schema is valid.

.. code-block::

    SELECT PC_SchemaIsValid(schema) FROM pointcloud_formats LIMIT 1;

    t
