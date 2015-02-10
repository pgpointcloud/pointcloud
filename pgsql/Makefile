# pointcloud

include ../config.mk

MODULE_big = pointcloud
OBJS = \
	pc_inout.o \
	pc_access.o \
	pc_pgsql.o

EXTENSION = pointcloud
DATA = pointcloud--1.0.sql

REGRESS = pointcloud pointcloud_columns

ifneq ($(GHT_LDFLAGS),)
REGRESS += pointcloud-ght
endif

# Add in build/link flags for lib
PG_CPPFLAGS += -I../lib
SHLIB_LINK += ../lib/$(LIB_A) $(filter -lm, $(LIBS)) $(XML2_LDFLAGS) $(ZLIB_LDFLAGS) $(GHT_LDFLAGS)

# We are going to use PGXS for sure
include $(PGXS)
