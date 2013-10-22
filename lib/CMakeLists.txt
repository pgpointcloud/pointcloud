
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
        pc_patch_uncompressed.c
        pc_point.c
        pc_pointlist.c
        pc_schema.c
        pc_stats.c
        pc_util.c
        pc_val.c
        )

set ( PC_HEADERS
        hashtable.h           
        stringbuffer.h    
        pc_api.h  
        pc_api_internal.h
        pc_config.h
        )

add_library (libpc-static STATIC ${PC_SOURCES} ${PC_HEADERS})

set_target_properties (libpc-static 
  PROPERTIES
    OUTPUT_NAME "pc"
    PREFIX "lib"
    CLEAN_DIRECT_OUTPUT 1
  )

target_link_libraries (libpc-static xml2)
target_link_libraries (libpc-static z)
if (LIBGHT_FOUND)
  target_link_libraries (libpc-static ght)
endif (LIBGHT_FOUND)


add_subdirectory (cunit)