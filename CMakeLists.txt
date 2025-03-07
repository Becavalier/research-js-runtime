cmake_minimum_required(VERSION 3.10)
project(tiny_nodejs)

set(CMAKE_CXX_STANDARD 17)
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

# Find libuv package
find_package(LibUV REQUIRED)
include_directories(${LIBUV_INCLUDE_DIR})

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(tiny_node ${SOURCES})

# Link libraries
target_link_libraries(tiny_node ${V8_LIBRARIES} ${LIBUV_LIBRARIES})

# Installation
install(TARGETS tiny_node DESTINATION bin)

# Print configuration information
message(STATUS "V8 include directory: ${V8_INCLUDE_DIR}")
message(STATUS "V8 libraries: ${V8_LIBRARIES}")
message(STATUS "LibUV include directory: ${LIBUV_INCLUDE_DIR}")
message(STATUS "LibUV libraries: ${LIBUV_LIBRARIES}") 