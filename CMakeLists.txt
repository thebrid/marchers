cmake_minimum_required(VERSION 3.25)

project(marchers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

function(marchers_apply_warnings target)
  target_compile_options(${target} PRIVATE
    -Wall
    -Werror
    -Wpedantic
    -pedantic-errors
    -Weffc++
  )
endfunction()

add_subdirectory(src)
add_subdirectory(unit_tests)
