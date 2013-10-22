

set ( PC_SOURCES
        pc_access.c 
        pc_inout.c      
        pc_pgsql.c       
        )

set ( PC_HEADERS
        pc_pgsql.h
        )

include_directories ("${PGSQL_INCLUDEDIR_SERVER}")
include_directories ("${PROJECT_SOURCE_DIR}/lib")

add_library (pointcloud MODULE ${PC_SOURCES} ${PC_HEADERS})

if (APPLE)
  set (PC_LINK_FLAGS "-bundle_loader ${PGSQL_BINDIR}/postgres")
else(APPLE)
  set (PC_LINK_FLAGS "")
endif (APPLE)

set_target_properties (pointcloud
  PROPERTIES
  OUTPUT_NAME "pointcloud"
  LINK_FLAGS ${PC_LINK_FLAGS}
  PREFIX ""
  )

target_link_libraries (pointcloud libpc-static)
target_link_libraries (pointcloud xml2)
target_link_libraries (pointcloud z)
if (LIBGHT_FOUND)
  target_link_libraries (pointcloud ght)
endif (LIBGHT_FOUND)

