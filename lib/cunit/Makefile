
include ../../config.mk

LIBPC = ../$(LIBPC_A)

CPPFLAGS = $(XML2_CPPFLAGS) $(CUNIT_CPPFLAGS) -I..
LDFLAGS = $(XML2_LDFLAGS) $(CUNIT_LDFLAGS)

EXE = cu_tester

# ADD YOUR NEW TEST FILE HERE (1/1)
OBJS =	\
	cu_tester.o \
	cu_pc_schema.o \
	cu_pc_point.o \
	cu_pc_patch.o

# Build the unit tester
all: $(EXE)

# Build and run the unit tester
check: $(EXE)
	@./$(EXE)

# Build the main unit test executable
$(EXE): $(OBJS) $(LIBPC)
	$(CC) $(LDFLAGS) -o $@ $^

$(LIBPC):
	$(MAKE) -C .. $(LIBPC_A)

# Clean target
clean:
	@rm -f $(OBJS)
	@rm -f $(EXE)
