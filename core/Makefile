
OBJS = \
	pc_core.o \
	pc_mem.o

LIB = pc_lib.a

all: $(LIB)

$(LIB): $(OBJS)
	ar rs $@ $^

clean:
	@rm -f $(OBJS) $(LIB)
	$(MAKE) -C cunit $@

check:
	$(MAKE) -C cunit $@
