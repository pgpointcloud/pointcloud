
include ../makeglobals.mk

CPPFLAGS = $(XML2_CPPFLAGS)
LDFLAGS = $(XML2_LDFLAGS)

OBJS = \
	pc_core.o \
	pc_mem.o \
	pc_patch.o \
	pc_point.o \
	pc_schema.o \
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

check:
	$(MAKE) -C cunit $@

