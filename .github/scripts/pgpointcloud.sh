#! /bin/bash

set -e

cd ../.. && \
    ./tools/build-install.sh && \
    sh .install-lazperf.sh && \
    ./tools/build-install.sh --with-lazperf=/usr/local && \
    make check && \
    ./tools/valgrind.sh && \
    make installcheck && \
    cat pgsql/regression.diffs || true
