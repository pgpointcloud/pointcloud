OBJS = pc_core.o

all: liblwgeom.a

liblwgeom.a: $(OBJS)
	ar rs $@ $^

clean:
	@rm -f *.o *.a *.so

