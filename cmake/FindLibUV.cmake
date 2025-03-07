# FindLibUV.cmake
# Find the libuv library
#
# This module defines:
#  LIBUV_FOUND - Whether libuv was found
#  LIBUV_INCLUDE_DIR - The libuv include directories
#  LIBUV_LIBRARIES - The libuv libraries

# Find include directory
find_path(LIBUV_INCLUDE_DIR
  NAMES uv.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /opt/homebrew/include
    $ENV{LIBUV_DIR}/include
)

# Find library
find_library(LIBUV_LIBRARIES
  NAMES uv libuv
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
    $ENV{LIBUV_DIR}/lib
)

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUV DEFAULT_MSG LIBUV_LIBRARIES LIBUV_INCLUDE_DIR)

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARIES) 