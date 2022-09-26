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


.. note::

  If ``qsort_r`` is not available on your system, then you can use the embedded
  implementation instead thanks to the next directive:

  .. code-block:: bash

    $ ./configure CFLAGS="-DNESTED_QSORT=1"

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
- ``sphinx_rtd_theme``

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

+--------------------+-----------------------+------------------------+
|                    | PostGIS 2.5           | PostGIS 3.2            |
+--------------------+-----------------------+------------------------+
| PostgreSQL 10      | |10_25_bionic|        |                        |
+--------------------+-----------------------+------------------------+
| PostgreSQL 11      | |11_25_bionic|        |                        |
+--------------------+-----------------------+------------------------+
| PostgreSQL 12      | |12_25_bionic|        | |12_32_bionic|         |
|                    |                       |                        |
|                    |                       | |12_32_focal|          |
+--------------------+-----------------------+------------------------+
| PostgreSQL 13      |                       | |13_32_focal|          |
+--------------------+-----------------------+------------------------+
| PostgreSQL 14      |                       | |14_32_focal|          |
+--------------------+-----------------------+------------------------+
| PostgreSQL 15      |                       | |15_32_focal|          |
+--------------------+-----------------------+------------------------+

.. |10_25_bionic| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-18.04%5D%20PostgreSQL%2010%20and%20PostGIS%202.5?label=Ubuntu%2018.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-18.04%5D+PostgreSQL+10+and+PostGIS+2.5%22

.. |11_25_bionic| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-18.04%5D%20PostgreSQL%2011%20and%20PostGIS%202.5?label=Ubuntu%2018.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-18.04%5D+PostgreSQL+11+and+PostGIS+2.5%22

.. |12_25_bionic| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-18.04%5D%20PostgreSQL%2012%20and%20PostGIS%202.5?label=Ubuntu%2018.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-18.04%5D+PostgreSQL+12+and+PostGIS+2.5%22

.. |12_32_bionic| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-18.04%5D%20PostgreSQL%2012%20and%20PostGIS%203.2?label=Ubuntu%2018.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-18.04%5D+PostgreSQL+12+and+PostGIS+3.2%22

.. |12_32_focal| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-20.04%5D%20PostgreSQL%2012%20and%20PostGIS%203.2?label=Ubuntu%2020.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-20.04%5D+PostgreSQL+12+and+PostGIS+3.2%22

.. |13_32_focal| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-20.04%5D%20PostgreSQL%2013%20and%20PostGIS%203.2?label=Ubuntu%2020.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-20.04%5D+PostgreSQL+13+and+PostGIS+3.2%22

.. |14_32_focal| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-20.04%5D%20PostgreSQL%2014%20and%20PostGIS%203.2?label=Ubuntu%2020.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-20.04%5D+PostgreSQL+14+and+PostGIS+3.2%22

.. |15_32_focal| image:: https://img.shields.io/github/workflow/status/pgpointcloud/pointcloud/%5Bubuntu-20.04%5D%20PostgreSQL%2015%20and%20PostGIS%203.2?label=Ubuntu%2020.04&logo=github&style=plastic :target: https://github.com/pgpointcloud/pointcloud/actions?query=workflow%3A%22%5Bubuntu-20.04%5D+PostgreSQL+15+and+PostGIS+3.2%22

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

------------------------------------------------------------------------------
Valgrind memcheck
------------------------------------------------------------------------------

For checking the memory management of pgPointcloud extension, ``Valgrind`` can
be used with ``PostgreSQL`` in single-user mode.

But first, it's necessary to compile the extension with debug symbols and
without compiler optimizations:

.. code-block:: console

  $ ./configure CFLAGS="-O0 -g"
  $ make
  $ sudo make install

Debug symbols may also be installed for PostgreSQL and PostGIS. For example
for Debian based distributions with PostgreSQL 13 and PostGIS 3:

.. code-block:: console

  $ sudo apt-get install postgresql-13-dbgsym postgresql-13-postgis-3-dbgsym

And finally:

.. code-block:: console

  $ echo "select pc_transform(patch, 1) from patchs limit 1" | \
    valgrind -s --track-origins=yes --leak-check=yes \
      --show-leak-kinds=all --read-var-info=yes --log-file=/tmp/valgrind.log \
      /usr/lib/postgresql/13/bin/postgres --single -D /var/lib/postgresql/13/main \
      -c config_file=/etc/postgresql/13/main/postgresql.conf \
      mydatabase

Then Valgrind's report is available in ``/tmp/valgrind.log``.

------------------------------------------------------------------------------
GDB interactive mode
------------------------------------------------------------------------------

GDB may be attached to a running session for debugging purpose:

.. code-block:: console

  $ psql mydatabase
  psql (14.5)
  Type "help" for help.

  mydatabase=# select pid from pg_stat_activity where usename = 'pblottiere' and state = 'active';
    pid
  -------
   34699
  (1 row)


.. code-block:: console

  $ sudo gdb -p 34699
  GNU gdb (GDB) 12.1
  (gdb)

Then you can execute a SQL request in the corresponding session and use GDB as
usual (step by step, backtrace, ...).
