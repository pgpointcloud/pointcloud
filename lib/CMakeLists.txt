
set ( PC_SOURCES
        hashtable.c 
        stringbuffer.c      
        pc_bytes.c       
        pc_dimstats.c      
        pc_filter.c    
        pc_mem.c 
        pc_patch.c
        pc_patch_dimensional.c
        pc_patch_ght.c
        pc_patch_lazperf.c
        pc_patch_uncompressed.c
        pc_point.c
        pc_pointlist.c
        pc_schema.c
        pc_sort.c
        pc_stats.c
        pc_util.c
        pc_val.c
        )

set ( LAZPERF_SOURCES
        lazperf_adapter.cpp
        )

set ( PC_HEADERS
        hashtable.h           
        stringbuffer.h    
        pc_api.h  
        pc_api_internal.h
        pc_config.h
        )

add_library (libpc-static STATIC ${PC_SOURCES} ${PC_HEADERS})
add_library (liblazperf-static STATIC ${LAZPERF_SOURCES})

set_target_properties (libpc-static 
  PROPERTIES
    OUTPUT_NAME "pc"
    PREFIX "lib"
    CLEAN_DIRECT_OUTPUT 1
    COMPILE_FLAGS "-fPIC -std=gnu89"
  )

set_target_properties (liblazperf-static
  PROPERTIES
    OUTPUT_NAME "lazperf"
    PREFIX "lib"
    CLEAN_DIRECT_OUTPUT 1
    COMPILE_FLAGS "-fPIC -std=c++0x"
  )

target_link_libraries (libpc-static xml2)
target_link_libraries (libpc-static z)
target_link_libraries (libpc-static m)
if (LIBGHT_FOUND)
  target_link_libraries (libpc-static ${LIBGHT_LIBRARY})
endif (LIBGHT_FOUND)
if (LAZPERF_FOUND)
  target_link_libraries (libpc-static liblazperf-static)
endif (LAZPERF_FOUND)

if (WITH_TESTS)
    add_subdirectory (cunit)
endif (WITH_TESTS)
