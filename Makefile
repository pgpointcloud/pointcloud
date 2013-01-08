# pointcloud

MODULE_big = pointcloud
OBJS = pc_core.a

EXTENSION = pointcloud
DATA = pointcloud--1.0.sql

REGRESS = pointcloud

SHLIB_LINK += $(filter -lm, $(LIBS))

# We are going to use PGXS for sure
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


pg_core.a: pc_core.o