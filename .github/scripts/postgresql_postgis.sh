#! /bin/bash

set -e

wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" |sudo tee /etc/apt/sources.list.d/pgdg.list
sudo apt-get update
sudo apt-get --purge remove postgresql\*
sudo apt-get install -q postgresql-server-dev-$POSTGRESQL_VERSION postgresql-client-$POSTGRESQL_VERSION postgresql-$POSTGRESQL_VERSION-postgis-$POSTGIS_VERSION libcunit1-dev valgrind g++
sudo pg_dropcluster --stop $POSTGRESQL_VERSION main
sudo rm -rf /etc/postgresql/$POSTGRESQL_VERSION /var/lib/postgresql/$POSTGRESQL_VERSION
sudo pg_createcluster -u postgres $POSTGRESQL_VERSION main --start -- --auth-local trust --auth-host trust
sudo /etc/init.d/postgresql start $POSTGRESQL_VERSION || sudo journalctl -xe
sudo -iu postgres psql -c 'CREATE ROLE runner SUPERUSER LOGIN CREATEDB;'
