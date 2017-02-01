#!/bin/sh
set -ex
git clone https://github.com/pramsey/libght.git
cd libght; cmake .; make; sudo make install; sudo ldconfig
