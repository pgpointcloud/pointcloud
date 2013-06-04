
include ../config.mk

CPPFLAGS = $(XML2_CPPFLAGS) $(ZLIB_CPPFLAGS) $(GHT_CPPFLAGS)
LDFLAGS = $(XML2_LDFLAGS) $(ZLIB_LDFLAGS) $(GHT_LDFLASGS)

OBJS = \
	pc_bytes.o \
	pc_dimstats.o \
	pc_filter.o \
	pc_mem.o \
	pc_patch.o \
	pc_patch_dimensional.o \
	pc_patch_uncompressed.o \
	pc_patch_ght.o \
	pc_point.o \
	pc_pointlist.o \
	pc_schema.o \
	pc_stats.o \
	pc_util.o \
	pc_val.o \
	stringbuffer.o \
	hashtable.o

all: $(LIB_A)
	$(MAKE) -C cunit $@

$(LIB_A): $(OBJS)
	ar rs $@ $^

clean:
	@rm -f $(OBJS) $(LIB_A)
	$(MAKE) -C cunit $@

install:
	@echo "No install target in lib"

check:
	$(MAKE) -C cunit $@

