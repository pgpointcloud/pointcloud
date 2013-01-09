# pointcloud

MODULE_big = pointcloud
OBJS = 

EXTENSION = pointcloud
DATA = pointcloud--1.0.sql

REGRESS = pointcloud

SHLIB_LINK += $(filter -lm, $(LIBS)) ../core/pc_lib.a

# We are going to use PGXS for sure
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
