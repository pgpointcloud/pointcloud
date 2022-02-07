#! /bin/bash

clang-format -i --style=llvm lib/*.c lib/*.h lib/cunit/*.c lib/*.cpp lib/*.hpp lib/cunit/*.h
