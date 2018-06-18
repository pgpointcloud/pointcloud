# pointcloud

include ../config.mk

OBJS = \
	pc_inout.o \
	pc_access.o \
	pc_editor.o \
	pc_pgsql.o

SED = sed
EXTENSION = pointcloud
EXTVERSION=$(shell cat ../Version.config)
EXTVERSION_MAJOR=$(shell cut -d. -f1,2 ../Version.config)
MODULE_big = $(EXTENSION)-$(EXTVERSION_MAJOR)
UPGRADABLE = 1.0 1.0.1

UPGRADES = \
  $(shell echo $(UPGRADABLE) | \
     $(SED) 's/^/$(EXTENSION)--/' | \
     $(SED) 's/$$/--$(EXTVERSION).sql/' | \
     $(SED) 's/ /--$(EXTVERSION).sql $(EXTENSION)--/g') \
  $(EXTENSION)--$(EXTVERSION)--$(EXTVERSION)next.sql \
  $(EXTENSION)--$(EXTVERSION)next--$(EXTVERSION).sql

DATA_built = \
  $(EXTENSION).control \
  $(EXTENSION)--$(EXTVERSION).sql \
  $(UPGRADES)

REGRESS = pointcloud pointcloud_columns schema

ifneq ("$(LAZPERF_STATUS)", "disabled")
REGRESS += pointcloud-laz
endif

# Add in build/link flags for lib
PG_CPPFLAGS += -I../lib
SHLIB_LINK += ../lib/$(LIB_A) ../lib/$(LIB_A_LAZPERF) -lstdc++ $(filter -lm, $(LIBS)) $(XML2_LDFLAGS) $(ZLIB_LDFLAGS)

# We are going to use PGXS for sure
include $(PGXS)

$(EXTENSION).control: $(EXTENSION).control.in Makefile
	$(SED) -e 's/@POINTCLOUD_VERSION@/$(EXTVERSION)/' \
         -e 's/@POINTCLOUD_VERSION_MAJOR@/$(EXTVERSION_MAJOR)/' $< > $@

$(EXTENSION)--$(EXTVERSION).sql: $(EXTENSION).sql.in Makefile
	$(SED) -e 's/@POINTCLOUD_VERSION@/$(EXTVERSION)/' $< > $@

# NOTE: relies on PERL being defined by PGXS
$(EXTENSION)--%--$(EXTVERSION).sql: $(EXTENSION)--$(EXTVERSION).sql ../util/proc_upgrade.pl
	cat $< | ../util/proc_upgrade.pl > $@

$(EXTENSION)--%--$(EXTVERSION)next.sql: $(EXTENSION)--$(EXTVERSION)next--$(EXTVERSION).sql
	ln -f $< $@
