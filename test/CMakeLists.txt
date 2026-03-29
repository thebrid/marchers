include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(marchers_test march_test.cpp)
target_compile_options(marchers_test PRIVATE -Wall -Wextra -Werror -pedantic-errors)
target_link_libraries(marchers_test PRIVATE GTest::gtest_main marchers_lib)
gtest_discover_tests(marchers_test)
