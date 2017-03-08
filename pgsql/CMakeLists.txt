

set ( PC_SOURCES
  pc_access.c 
  pc_inout.c      
  pc_pgsql.c       
  )

set ( PC_HEADERS
  pc_pgsql.h
  )

set ( PC_INSTALL_EXENSIONS
  "${PROJECT_BINARY_DIR}/pgsql/pointcloud--${POINTCLOUD_VERSION}.sql"
  "${PROJECT_BINARY_DIR}/pgsql/pointcloud.control"
  )

set ( PC_SQLS
  pointcloud.sql.in
  )

# create a "cat" function, used for concatenating the sql.in files
# in PC_SQLS into one sql.in file
function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

# concatenate the sql.in files
file(WRITE pointcloud--${POINTCLOUD_VERSION}.sql.in "")
foreach(SQL ${PC_SQLS})
  cat(${SQL} pointcloud--${POINTCLOUD_VERSION}.sql.in)
endforeach()

configure_file(
    pointcloud--${POINTCLOUD_VERSION}.sql.in
    "${PROJECT_BINARY_DIR}/pgsql/pointcloud--${POINTCLOUD_VERSION}.sql"
    )

configure_file(
    pointcloud.control.in
    "${PROJECT_BINARY_DIR}/pgsql/pointcloud.control"
    )

set(CMAKE_C_FLAGS "${PGSQL_CFLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${PGSQL_LDFLAGS}")

include_directories ("${PGSQL_INCLUDEDIR_SERVER}")
include_directories ("${PROJECT_SOURCE_DIR}/lib")

add_library (pointcloud MODULE ${PC_SOURCES} ${PC_HEADERS})

target_link_libraries (pointcloud libpc-static)

set_target_properties (pointcloud PROPERTIES
  OUTPUT_NAME "pointcloud-${POINTCLOUD_VERSION_MAJOR}"
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

