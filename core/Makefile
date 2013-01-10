
CC = gcc
CFLAGS = -I/opt/local/include/libxml2
LDFLAGS = -lxml2

OBJS = \
	pc_core.o \
	pc_mem.o \
	pc_schema.o

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
