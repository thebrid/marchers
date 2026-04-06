include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(marchers_unit_tests truth_test.cpp)
target_link_libraries(marchers_unit_tests PRIVATE GTest::gtest_main)
marchers_apply_warnings(marchers_unit_tests)

include(GoogleTest)
gtest_discover_tests(marchers_unit_tests)
