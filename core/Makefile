OBJS = pc_core.o
LIB = pc_lib.a

all: $(LIB)

$(LIB): $(OBJS)
	ar rs $@ $^

clean:
	@rm -f $(OBJS) $(LIB)

