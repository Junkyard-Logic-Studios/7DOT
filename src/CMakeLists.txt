project(7DOT)

# get source files
file(GLOB_RECURSE source_files "*.cpp")
list(APPEND source_files "../external/PugiXML/pugixml.cpp")
message(STATUS ${source_files})

# add executable target
add_executable(7dot ${source_files})
target_include_directories(7dot PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(7dot PRIVATE "../external/PugiXML")
target_link_libraries(7dot PRIVATE SDL3::SDL3)
