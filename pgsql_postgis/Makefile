# pointcloud

include ../config.mk

#MODULE_big = pointcloud_postgis
#OBJS =
EXTENSION = pointcloud_postgis
DATA = $(EXTENSION)--1.0.sql

#REGRESS = pointcloud

# Add in build/link flags for lib
#PG_CPPFLAGS += -I../lib
#SHLIB_LINK += $(filter -lm, $(LIBS)) $(XML2_LDFLAGS) $(ZLIB_LDFLAGS) ../lib/$(LIB_A)

# We are going to use PGXS for sure
include $(PGXS)
