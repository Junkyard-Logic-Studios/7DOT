project(7DOT)

# create game executable target
add_executable(hello WIN32 hello.c)

# link to the SDL3 library
target_link_libraries(hello PRIVATE SDL3::SDL3)
