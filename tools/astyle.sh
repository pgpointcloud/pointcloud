#!/bin/sh

exec astyle \
       --style=allman \
       --indent=force-tab=4 \
       --lineend=linux \
       --pad-oper \
       --pad-header \
       --unpad-paren \
       --suffix=none \
       --recursive \
       --formatted \
       --exclude=lib/sort_r \
       --exclude=lib/hashtable.c \
       --exclude=lib/hashtable.h \
       *.c *.h
