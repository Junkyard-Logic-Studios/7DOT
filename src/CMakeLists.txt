project(7DOT)

file(GLOB_RECURSE source_files "*.cpp")
message(STATUS ${source_files})
add_executable(7dot ${source_files})
target_link_libraries(7dot PRIVATE SDL3::SDL3 SDL3_net::SDL3_net)
