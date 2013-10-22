#------------------------------------------------------------------------------
# test suite build
#------------------------------------------------------------------------------

set (PC_TEST_SOURCES 
  cu_pc_patch_ght.c
  cu_pc_point.c
  cu_pc_bytes.c
  cu_pc_schema.c
  cu_pc_patch.c
  cu_tester.c
  )

set (PC_TEST_HEADERS
  cu_tester.h
  )

set (GHT_TEST_DATA
  data/las-schema.xml
  data/simple-schema-fine.xml
  data/pdal-schema.xml
  data/simple-schema.xml
  )

include_directories ("${PROJECT_SOURCE_DIR}/lib")
include_directories ("${CUNIT_INCLUDE_DIR}")

add_executable(cu_tester ${PC_TEST_SOURCES} ${PC_TEST_HEADERS})
target_link_libraries (cu_tester libpc-static ${CUNIT_LIBRARIES})

add_test(cu_tester cu_tester)

