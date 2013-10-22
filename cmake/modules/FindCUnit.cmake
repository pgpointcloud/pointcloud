# Find the CUnit headers and libraries
#
#  CUNIT_INCLUDE_DIRS - The CUnit include directory (directory where CUnit/CUnit.h was found)
#  CUNIT_LIBRARIES    - The libraries needed to use CUnit
#  CUNIT_FOUND        - True if CUnit found in system
 
 
FIND_PATH(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)
 
FIND_LIBRARY(CUNIT_LIBRARY NAMES 
    cunit
    libcunit
    cunitlib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CUnit DEFAULT_MSG CUNIT_LIBRARY CUNIT_INCLUDE_DIR)
 
IF(CUNIT_FOUND)
  SET(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
  SET(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})
ENDIF(CUNIT_FOUND)

MARK_AS_ADVANCED(CLEAR CUNIT_INCLUDE_DIR)
MARK_AS_ADVANCED(CLEAR CUNIT_LIBRARY)
 
