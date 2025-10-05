cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(7DOT)

# Set the output directory for build objects
# This makes sure that the dynamic library goes into the build directory automatically
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Build
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(src)

option(BUILD_TESTS "Build tests or not" ON)
if(BUILD_TESTS)
	message(STATUS "Building tests...")
	add_subdirectory(tests)
endif()

# Install
