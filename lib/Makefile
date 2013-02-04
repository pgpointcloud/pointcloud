
include ../config.mk

CPPFLAGS = $(XML2_CPPFLAGS)
LDFLAGS = $(XML2_LDFLAGS)

OBJS = \
	pc_dimensional.o \
	pc_mem.o \
	pc_patch.o \
	pc_point.o \
	pc_pointlist.o \
	pc_schema.o \
	pc_util.o \
	pc_val.o \
	stringbuffer.o \
	hashtable.o

all: $(LIBPC_A)
	$(MAKE) -C cunit $@

$(LIBPC_A): $(OBJS)
	ar rs $@ $^

clean:
	@rm -f $(OBJS) $(LIBPC_A)
	$(MAKE) -C cunit $@

install:
	@echo "No install target in lib"

check:
	$(MAKE) -C cunit $@

