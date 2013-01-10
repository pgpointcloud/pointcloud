
OBJS = \
	pc_core.o \
	pc_mem.o \
	pc_schema.o

LIB = pc_lib.a

all: $(LIB)
	$(MAKE) -C cunit $@

$(LIB): $(OBJS)
	ar rs $@ $^

clean:
	@rm -f $(OBJS) $(LIB)
	$(MAKE) -C cunit $@

check:
	$(MAKE) -C cunit $@
