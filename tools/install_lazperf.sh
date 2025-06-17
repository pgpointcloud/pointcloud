#!/bin/sh
set -ex
git clone https://github.com/hobuinc/laz-perf.git
cd laz-perf; git checkout 1.5.0; cmake -DWITH_TESTS=FALSE .; make; sudo make install
