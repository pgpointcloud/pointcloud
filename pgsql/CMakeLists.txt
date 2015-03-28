

set ( PC_SOURCES
  pc_access.c 
  pc_inout.c      
  pc_pgsql.c       
  )

set ( PC_HEADERS
  pc_pgsql.h
  )

set ( PC_INSTALL_EXENSIONS
  pointcloud--${POINTCLOUD_VERSION}.sql
  pointcloud.control
  )

configure_file(
    pointcloud.sql.in
    ${CMAKE_CURRENT_LIST_DIR}/pointcloud--${POINTCLOUD_VERSION}.sql
    )

configure_file(
    pointcloud.control.in
    ${CMAKE_CURRENT_LIST_DIR}/pointcloud.control
    )

set(CMAKE_C_FLAGS "${PGSQL_CPPFLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${PGSQL_LDFLAGS}")

include_directories ("${PGSQL_INCLUDEDIR_SERVER}")
include_directories ("${PROJECT_SOURCE_DIR}/lib")

add_library (pointcloud MODULE ${PC_SOURCES} ${PC_HEADERS})

target_link_libraries (pointcloud libpc-static)
target_link_libraries (pointcloud xml2)
target_link_libraries (pointcloud z)

if (LIBGHT_FOUND)
  target_link_libraries (pointcloud ght)
endif (LIBGHT_FOUND)

set_target_properties (pointcloud PROPERTIES
  OUTPUT_NAME "pointcloud"
  PREFIX ""
  )

if (APPLE)
  set_target_properties ( 
    pointcloud PROPERTIES
    LINK_FLAGS "-bundle_loader ${PGSQL_BINDIR}/postgres"
    )
endif (APPLE)

if (WIN32)
  include_directories ("${PGSQL_INCLUDEDIR_SERVER}/port/win32")
  if (MSVC)
    set_target_properties(pointcloud PROPERTIES COMPILE_FLAGS "-DHAVE_GETHOSTNAME")
  endif(MSVC)
  if (MINGW)
    target_link_libraries (pointcloud postgres)
  endif (MINGW)
endif (WIN32)


install (
  TARGETS pointcloud 
  DESTINATION ${PGSQL_PKGLIBDIR}
  )
  
install (
  FILES ${PC_INSTALL_EXENSIONS} 
  DESTINATION "${PGSQL_SHAREDIR}/extension"
  )

