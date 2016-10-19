#------------------------------------------------------------------------------
# CMakeLists.txt - root CMake configuration file
# Copyright (C) 2013 Boundless


#------------------------------------------------------------------------------
# versions

project(POINTCLOUD)
file(READ "Version.config" POINTCLOUD_VERSION)
string(STRIP ${POINTCLOUD_VERSION} POINTCLOUD_VERSION)
set (POINTCLOUD_VERSION "${POINTCLOUD_VERSION}")
string(REGEX REPLACE "\\.[0-9]*$" "" POINTCLOUD_VERSION_MAJOR "${POINTCLOUD_VERSION}")
set (POINTCLOUD_VERSION_MAJOR "${POINTCLOUD_VERSION_MAJOR}")


#------------------------------------------------------------------------------
# internal cmake settings

cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

set (CMAKE_COLOR_MAKEFILE ON)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING 
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()
message(STATUS "Setting ${CMAKE_PROJECT_NAME} build type - ${CMAKE_BUILD_TYPE}")


#------------------------------------------------------------------------------
# options
option(WITH_TESTS "Choose if CUnit tests should be built" TRUE)


#------------------------------------------------------------------------------
# check for headers

include (CheckIncludeFiles)

check_include_files (stdint.h HAVE_STDINT_H)
check_include_files (getopt.h HAVE_GETOPT_H)


#------------------------------------------------------------------------------
# PostgreSQL server paths come from pg_config
#

find_program(PG_CONFIG pg_config)
if(NOT PG_CONFIG)
  message(FATAL ERROR "Unable to find 'pg_config'")
endif(NOT PG_CONFIG)

exec_program(${PG_CONFIG} ARGS --version OUTPUT_VARIABLE PGCONFIG_OUTPUT)

string(REGEX REPLACE 
    "^PostgreSQL[\t ]+([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1.\\2.\\3"
    PGSQL_VERSION "${PGCONFIG_OUTPUT}")

unset(PGCONFIG_OUTPUT)

if(PGSQL_VERSION)
    string(REGEX REPLACE
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
        PGSQL_VERSION_MAJOR "${PGSQL_VERSION}")
    string(REGEX REPLACE
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
        PGSQL_VERSION_MINOR "${PGSQL_VERSION}")

    set(PGSQL_NUMERIC_VERSION ${PGSQL_VERSION_MAJOR}${PGSQL_VERSION_MINOR})
endif()

exec_program(${PG_CONFIG} ARGS --includedir
    OUTPUT_VARIABLE PGSQL_INCLUDEDIR)
exec_program(${PG_CONFIG} ARGS --includedir-server
    OUTPUT_VARIABLE PGSQL_INCLUDEDIR_SERVER)
    
exec_program(${PG_CONFIG} ARGS --pkglibdir OUTPUT_VARIABLE PGSQL_PKGLIBDIR)
exec_program(${PG_CONFIG} ARGS --sharedir OUTPUT_VARIABLE PGSQL_SHAREDIR)
exec_program(${PG_CONFIG} ARGS --bindir OUTPUT_VARIABLE PGSQL_BINDIR)
exec_program(${PG_CONFIG} ARGS --cflags OUTPUT_VARIABLE PGSQL_CFLAGS)
exec_program(${PG_CONFIG} ARGS --ldflags OUTPUT_VARIABLE PGSQL_LDFLAGS)
exec_program(${PG_CONFIG} ARGS --libs OUTPUT_VARIABLE PGSQL_LIBS)

#------------------------------------------------------------------------------
# libxml2

find_package (LibXml2 REQUIRED)
mark_as_advanced (CLEAR LIBXML2_INCLUDE_DIR)
mark_as_advanced (CLEAR LIBXML2_LIBRARIES)
include_directories (${LIBXML2_INCLUDE_DIR})


#------------------------------------------------------------------------------
# zlib

find_package (ZLIB REQUIRED)
include_directories (${ZLIB_INCLUDE_DIR})


#------------------------------------------------------------------------------
# cunit, ght and lazperf

if (WITH_TESTS)
    find_package (CUnit)
endif (WITH_TESTS)

find_package (LibGHT)

if (LIBGHT_FOUND)
  set (HAVE_LIBGHT 1)
endif (LIBGHT_FOUND)

find_package (LazPerf)

if (LAZPERF_FOUND)
  set (HAVE_LAZPERF 1)
endif (LAZPERF_FOUND)

#------------------------------------------------------------------------------
# use default install location if not specified

if (NOT DEFINED LIB_INSTALL_DIR)
  set (LIB_INSTALL_DIR lib)
endif()

#------------------------------------------------------------------------------
# generate config include

configure_file (
  "${PROJECT_SOURCE_DIR}/lib/pc_config.h.cmake"
  "${PROJECT_BINARY_DIR}/lib/pc_config.h"
  )

include_directories ("${PROJECT_BINARY_DIR}/lib")


#------------------------------------------------------------------------------
# libpc is required for the database component

add_subdirectory (lib)
add_subdirectory (pgsql)
add_subdirectory (pgsql_postgis)



