#! /bin/bash

clang-format -i --style=llvm lib/*.c lib/*.h lib/cunit/*.c lib/cunit/*.h
