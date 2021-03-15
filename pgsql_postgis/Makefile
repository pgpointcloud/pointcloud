# pointcloud

include ../config.mk

#MODULE_big = pointcloud_postgis
#OBJS =
SED = sed
EXTENSION = pointcloud_postgis
EXTVERSION=$(shell cat ../Version.config)
DATA_built = \
  $(EXTENSION).control \
  $(EXTENSION)--$(EXTVERSION).sql

REGRESS += pointcloud_postgis

# Add in build/link flags for lib
#PG_CPPFLAGS += -I../lib
#SHLIB_LINK += $(filter -lm, $(LIBS)) $(XML2_LDFLAGS) $(ZLIB_LDFLAGS) ../lib/$(LIB_A)

# We are going to use PGXS for sure
include $(PGXS)

$(EXTENSION).control: $(EXTENSION).control.in Makefile
	$(SED) -e 's/@POINTCLOUD_VERSION@/$(EXTVERSION)/' $< > $@

$(EXTENSION)--$(EXTVERSION).sql: $(EXTENSION).sql.in Makefile
	$(SED) -e 's/@POINTCLOUD_VERSION@/$(EXTVERSION)/' $< > $@
