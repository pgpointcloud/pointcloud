#! /bin/bash

clang-format -i -style=file lib/*.c lib/*.h lib/cunit/*.c lib/*.cpp lib/*.hpp lib/cunit/*.h
