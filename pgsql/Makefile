# pointcloud

include ../config.mk

MODULE_big = pointcloud
OBJS = \
	pc_inout.o \
	pc_access.o \
	pc_pgsql.o

SED = sed
EXTENSION = pointcloud
EXTVERSION=$(shell cat ../Version.config)
UPGRADABLE = 1.0

UPGRADES = \
  $(shell echo $(UPGRADABLE) | \
     $(SED) 's/^/$(EXTENSION)--/' | \
     $(SED) 's/$$/--$(EXTVERSION).sql/' | \
     $(SED) 's/ /--$(EXTVERSION).sql $(EXTENSION)--/g')

DATA_built = \
  $(EXTENSION).control \
  $(EXTENSION)--$(EXTVERSION).sql \
  $(UPGRADES)

REGRESS = pointcloud pointcloud_columns

ifneq ($(GHT_LDFLAGS),)
REGRESS += pointcloud-ght
endif

# Add in build/link flags for lib
PG_CPPFLAGS += -I../lib
SHLIB_LINK += ../lib/$(LIB_A) $(filter -lm, $(LIBS)) $(XML2_LDFLAGS) $(ZLIB_LDFLAGS) $(GHT_LDFLAGS)

# We are going to use PGXS for sure
include $(PGXS)

$(EXTENSION).control: $(EXTENSION).control.in Makefile
	$(SED) -e 's/@@VERSION@@/$(EXTVERSION)/' $< > $@

$(EXTENSION)--%.sql: $(EXTENSION).sql.in Makefile
	cp $< $@

