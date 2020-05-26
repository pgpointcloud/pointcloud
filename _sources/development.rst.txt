.. _development_index:

******************************************************************************
Development
******************************************************************************

Developer documentation with dependancies, build instructions, how to build a
Docker image, update the documentation and run unit tests.

------------------------------------------------------------------------------
Requirements
------------------------------------------------------------------------------

- PostgreSQL and PostgreSQL development packages must be installed (pg_config
  and server headers). For Red Hat and Ubuntu, the package names are usually
  ``postgresql-dev`` or ``postgresql-devel``
- LibXML2 development packages must be installed, usually ``libxml2-dev`` or
  ``libxml2-devel``
- CUnit packages must be installed
- [Optional] ``laz-perf`` library may be installed for LAZ compression support
  (built from source_)

------------------------------------------------------------------------------
Build/Install
------------------------------------------------------------------------------

After generating the configure script with ``autogen.sh``, you can use
``./configure --help`` to get a complete listing of configuration options.

.. code-block:: bash

  $ ./autogen.sh
  $ ./configure
  $ make
  $ sudo make install

.. note::

      You can use ``--with-pgconfig`` on the ``./configure`` command line if
      you have multiple PostgreSQL installations on your system and want to target a
      specific one. For example:

      .. code-block:: bash

        $ ./configure --with-pgconfig=/usr/lib/postgresql/12/bin/pg_config

------------------------------------------------------------------------------
Tests
------------------------------------------------------------------------------

**Unit tests**

.. code-block:: bash

  $ make check


**Regressions tests**

pgPointcloud includes SQL tests to run against an existing installation. To run
the SQL tests:

.. code-block:: bash

  $ sudo make install
  $ PGUSER=a_user PGPASSWORD=a_password PGHOST=localhost make installcheck

This command will create a database named ``contrib_regression`` and will execute
the SQL scripts located in ``pgsql/sql`` in this database.

------------------------------------------------------------------------------
Write a loading system
------------------------------------------------------------------------------

If you are writing your own loading system and want to write into Pointcloud
types, create well-known binary inputs, in uncompressed format. If your schema
indicates that your patch storage is compressed, Pointcloud will automatically
compress your patch before storing it, so you can create patches in
uncompressed WKB without worrying about the nuances of particular internal
compression schemes.

The only issues to watch when creating WKB patches are: ensuring the data you
write is sized according to the schema (use the specified dimension type);
ensuring that the endianness of the data matches the declared endianness of the
patch.

------------------------------------------------------------------------------
Documentation
------------------------------------------------------------------------------

Sphinx is used to build the documentation. For that, you have to install the
next Python packages:

- ``sphinx``
- ``sphinxcontrib.bibtex``

Then:

.. code-block:: bash

  $ cd doc && make html

The HTML documentation is available in ``doc/build/html``.

.. note::

      The documentation can be generated in another format like pdf, epub, ...
      You can use ``make`` to get a list of all available formats.

------------------------------------------------------------------------------
Docker Image
------------------------------------------------------------------------------

A ``Dockerfile`` is provided in the ``docker`` directory and based on the
official PostgreSQL docker image available DockerHub_. The image generated
is based on PostgreSQL 12, PostGIS 3 and the laz-perf support is activated.

.. code-block:: bash

  $ docker build --rm -t pgpointcloud docker/

------------------------------------------------------------------------------
Continuous Integration
------------------------------------------------------------------------------

pgPointcloud tests are run with `Github Actions`_ on several Ubuntu versions
and with various PostgreSQL/PostGIS releases:

+---------------+----------------+-----------------+
|               | PostGIS 2.5    | PostGIS 3       |
+---------------+----------------+-----------------+
| PostgreSQL 9.6| |96_25|        |                 |
+---------------+----------------+-----------------+
| PostgreSQL 10 | |10_25|        |                 |
+---------------+----------------+-----------------+
| PostgreSQL 11 | |11_25|        |                 |
+---------------+----------------+-----------------+
| PostgreSQL 12 | |12_25|        | |12_3|          |
+---------------+----------------+-----------------+

 .. |96_25| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-16.04%5D%20PostgreSQL%209.6%20and%20PostGIS%202.5?label=Ubuntu%2016.04&logo=github&style=plastic
    :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-16.04%5D+PostgreSQL+9.6+and+PostGIS+2.5%22

.. |10_25| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-16.04%5D%20PostgreSQL%2010%20and%20PostGIS%202.5?label=Ubuntu%2016.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-16.04%5D+PostgreSQL+10+and+PostGIS+2.5%22

.. |11_25| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-16.04%5D%20PostgreSQL%2011%20and%20PostGIS%202.5?label=Ubuntu%2016.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-16.04%5D+PostgreSQL+11+and+PostGIS+2.5%22

.. |12_25| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-18.04%5D%20PostgreSQL%2012%20and%20PostGIS%202.5?label=Ubuntu%2018.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-18.04%5D+PostgreSQL+12+and+PostGIS+2.5%22

.. |12_3| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-18.04%5D%20PostgreSQL%2012%20and%20PostGIS%203?label=Ubuntu%2018.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-18.04%5D+PostgreSQL+12+and+PostGIS+3%22

.. _`source`: https://github.com/hobu/laz-perf
.. _`DockerHub`: https://hub.docker.com/_/postgres
.. _`GitHub Actions`: https://github.com/pgpointcloud/pointcloud/actions

------------------------------------------------------------------------------
Release
------------------------------------------------------------------------------

Steps for releasing a new version of Pointcloud:

1. Add a new section to the ``NEWS`` file, listing all the changes associated
   with the new release.

2. Change the version number in the ``Version.config`` and
   ``pgsql/expected/pointcloud.out`` files.

3. Update the value of ``UPGRADABLE`` in ``pgsql/Makefile.in``. This variable
   defines the versions from which a database can be upgraded to the new
   Pointcloud version.

4. Create a PR with these changes.

5. When the PR is merged create a tag for the new release and push it to
   GitHub:

.. code-block:: bash

  $ git tag -a vx.y.z -m 'version x.y.z'
  $ git push origin vx.y.z
