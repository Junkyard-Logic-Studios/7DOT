cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(7DOT)


### BUILD
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(DEBUG TRUE)
else()
	set(RELEASE TRUE)
endif()

# track asset directory
set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/")

# make defines accessible to source code
configure_file("configure.hpp.in" "configure.hpp")

# Set the output directory for build objects
# This makes sure that the dynamic library goes into the build directory automatically
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

# add SDL
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

# add GLM
add_subdirectory(external/glm)

# add own source directory
add_subdirectory(src)

# add tests
option(BUILD_TESTS "Build tests or not" OFF)
if(BUILD_TESTS)
	message(STATUS "Building tests...")
	add_subdirectory(tests)
endif()


### INSTALL
