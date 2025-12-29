project(7DOT)

include(FetchContent)

# Fetch Googletest
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  7dot_test
  inputBuffer.cpp
  level.cpp "../src/fight/level.cpp"
  "../external/PugiXML/pugixml.cpp"
)
target_include_directories(
  7dot_test PRIVATE
  "../src"
  "../external/PugiXML"
  "${CMAKE_BINARY_DIR}"
)
target_link_libraries(
  7dot_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(7dot_test)
