
PC_CPPFLAGS = -I../
PC_LIB = ../libpc.a

XML2_CPPFLAGS = -I/opt/local/include/libxml2
XML2_LDFLAGS = -lxml2

CUNIT_CPPFLAGS = -I/usr/local/include
CUNIT_LDFLAGS = -L/usr/local/lib -lcunit

CC = gcc
CFLAGS = -g -O0
CPPFLAGS = $(PC_CPPFLAGS) $(XML2_CPPFLAGS) $(CUNIT_CPPFLAGS)
LDFLAGS = $(XML2_LDFLAGS) $(CUNIT_LDFLAGS)

EXE = cu_tester

# ADD YOUR NEW TEST FILE HERE (1/1)
OBJS =	\
	cu_tester.o \
	cu_pc_schema.o 

# If we couldn't find the cunit library then display a helpful message
ifeq ($(CUNIT_LDFLAGS),)
all: requirements_not_met_cunit
check: requirements_not_met_cunit
else

# Build the unit tester
all: $(EXE)

# Build and run the unit tester
check: $(EXE)
	@./$(EXE)

endif

# Build the main unit test executable
$(EXE): $(OBJS) $(PC_LIB)
	$(CC) $(LDFLAGS) -o $@ $^

$(PC_LIB):
	$(MAKE) -C .. libpc.a

# Clean target
clean:
	@rm -f $(OBJS)
	@rm -f $(EXE)


# Requirements message
requirements_not_met_cunit:
	@echo
	@echo "WARNING:"
	@echo
	@echo "configure was unable to find CUnit which is required for unit testing."
	@echo "In order to enable unit testing, you must install CUnit and then re-run configure."
	@echo
