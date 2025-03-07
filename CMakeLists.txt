cmake_minimum_required(VERSION 3.10)
project(tiny_nodejs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the cmake directory to the module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find V8 package
find_package(V8 REQUIRED)
include_directories(${V8_INCLUDE_DIR})

# Add V8 specific compiler flags
add_definitions(-DV8_COMPRESS_POINTERS)
add_definitions(-DV8_31BIT_SMIS_ON_64BIT_ARCH)
add_definitions(-DV8_ENABLE_SANDBOX)

# Find libuv package
find_package(LibUV REQUIRED)
include_directories(${LIBUV_INCLUDE_DIR})

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(tiny_node ${SOURCES})

# Additional dependencies required for static linking with V8
if(APPLE)
  # macOS specific dependencies
  find_library(CORE_FOUNDATION CoreFoundation)
  find_library(SECURITY Security)
  target_link_libraries(tiny_node ${CORE_FOUNDATION} ${SECURITY})
  
  # Additional compiler flags for macOS
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Link libraries - static linking for V8
target_link_libraries(tiny_node 
  ${V8_LIBRARIES}
  ${LIBUV_LIBRARIES}
  pthread
  dl
)

# Installation
install(TARGETS tiny_node DESTINATION bin)

# Print configuration information
message(STATUS "V8 include directory: ${V8_INCLUDE_DIR}")
message(STATUS "V8 libraries: ${V8_LIBRARIES}")
message(STATUS "LibUV include directory: ${LIBUV_INCLUDE_DIR}")
message(STATUS "LibUV libraries: ${LIBUV_LIBRARIES}") 