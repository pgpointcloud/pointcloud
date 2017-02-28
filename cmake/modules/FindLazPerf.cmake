# Find the LazPerf headers
#
#  LAZPERF_INCLUDE_DIRS - The LazPerf include directory (directory where laz-perf/las.hpp was found)
#  LAZPERF_FOUND        - True if LazPerf found in system


FIND_PATH(LAZPERF_INCLUDE_DIR NAMES laz-perf/las.hpp)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LazPerf DEFAULT_MSG LAZPERF_INCLUDE_DIR)

IF(LAZPERF_FOUND)
  SET(LAZPERF_INCLUDE_DIRS ${LAZPERF_INCLUDE_DIR})
ENDIF(LAZPERF_FOUND)

MARK_AS_ADVANCED(CLEAR LAZPERF_INCLUDE_DIR)
