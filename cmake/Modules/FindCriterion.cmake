# Find Criterion library
#
#  This module defines the following variables:
#     CRITERION_FOUND        - True if Criterion is found
#     CRITERION_LIBRARIES    - Criterion libraries
#     CRITERION_INCLUDE_DIRS - Criterion include directories
#

find_package(PkgConfig)
pkg_check_modules(PC_CRITERION QUIET criterion)
find_path(CRITERION_INCLUDE_DIRS NAMES criterion.h HINTS ${PC_CRITERION_INCLUDE_DIRS} PATH_SUFFIXES criterion)
find_library(CRITERION_LIBRARIES NAMES criterion HINTS ${PC_CRITERION_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Criterion DEFAULT_MSG CRITERION_LIBRARIES CRITERION_INCLUDE_DIRS)
mark_as_advanced(CRITERION_LIBRARIES CRITERION_INCLUDE_DIRS)
