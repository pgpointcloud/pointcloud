#! /bin/bash

pg_dump contrib_regression -Fp > dump.sql
cat dump.sql
createdb test_restore
psql test_restore < dump.sql
