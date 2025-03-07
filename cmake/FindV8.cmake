# FindV8.cmake
# Find the V8 JavaScript engine
#
# This module defines:
#  V8_FOUND - Whether V8 was found
#  V8_INCLUDE_DIR - The V8 include directories
#  V8_LIBRARIES - The V8 libraries

# Use the static library path provided by the user
set(V8_STATIC_LIB_PATH "/Users/jasonyu/Repo/v8-home/v8/out.gn/arm64.release/obj/libv8_monolith.a")

# Find include directory
find_path(V8_INCLUDE_DIR
  NAMES v8.h
  PATHS
    /Users/jasonyu/Repo/v8-home/v8/include
    /usr/include
    /usr/local/include
    /opt/local/include
    /opt/homebrew/include
    $ENV{V8_DIR}/include
)

# Set the library directly to the static library path
set(V8_LIBRARIES ${V8_STATIC_LIB_PATH})

# Check if the static library exists
if(NOT EXISTS ${V8_STATIC_LIB_PATH})
  message(FATAL_ERROR "V8 static library not found at ${V8_STATIC_LIB_PATH}")
endif()

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8 DEFAULT_MSG V8_LIBRARIES V8_INCLUDE_DIR)

mark_as_advanced(V8_INCLUDE_DIR V8_LIBRARIES) 