# FindV8.cmake
# Find the V8 JavaScript engine
#
# This module defines:
#  V8_FOUND - Whether V8 was found
#  V8_INCLUDE_DIR - The V8 include directories
#  V8_LIBRARIES - The V8 libraries

# Find include directory
find_path(V8_INCLUDE_DIR
  NAMES v8.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /opt/homebrew/include
    $ENV{V8_DIR}/include
)

# Find library
find_library(V8_LIBRARIES
  NAMES v8 libv8
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
    $ENV{V8_DIR}/lib
)

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8 DEFAULT_MSG V8_LIBRARIES V8_INCLUDE_DIR)

mark_as_advanced(V8_INCLUDE_DIR V8_LIBRARIES) 