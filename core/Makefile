
CC = gcc
CFLAGS = -g -O0

XML_CPPFLAGS = -I/opt/local/include/libxml2
XML_LDFLAGS = -lxml2

CPPFLAGS = $(XML_CPPFLAGS)
LDFLAGS = $(XML_LDFLAGS)

OBJS = \
	pc_core.o \
	pc_mem.o \
	pc_val.o \
	pc_schema.o \
	pc_point.o \
	stringbuffer.o \
	hashtable.o

LIB = libpc.a

all: $(LIB)
	$(MAKE) -C cunit $@

$(LIB): $(OBJS)
	ar rs $@ $^

clean:
	@rm -f $(OBJS) $(LIB)
	$(MAKE) -C cunit $@

check:
	$(MAKE) -C cunit $@

