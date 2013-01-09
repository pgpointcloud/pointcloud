
PC_LIB = ../pc_lib.a
PC_INC = -I../

CUNIT_LDFLAGS = -L/usr/local/lib -lcunit
CUNIT_CPPFLAGS = -I/usr/local/include -I..

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
all: cu_tester

# Build and run the unit tester
check: cu_tester
	@./cu_tester

endif

# Build the main unit test executable
cu_tester: $(OBJS) $(PC_LIB)
	$(CC) $(CUNIT_LDFLAGS) -o $@ $^

# Command to build each of the .o files
$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(CUNIT_CPPFLAGS) -c -o $@ $<

$(PC_LIB):
	$(MAKE) -C ../ pc_lib.a

# Clean target
clean:
	@rm -f $(OBJS)
	@rm -f cu_tester


# Requirements message
requirements_not_met_cunit:
	@echo
	@echo "WARNING:"
	@echo
	@echo "configure was unable to find CUnit which is required for unit testing."
	@echo "In order to enable unit testing, you must install CUnit and then re-run configure."
	@echo
