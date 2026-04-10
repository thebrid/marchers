add_library(marchers_lib
  decompress.cpp
)

target_include_directories(marchers_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(marchers main.cpp)
target_link_libraries(marchers PRIVATE marchers_lib)
marchers_apply_warnings(marchers)
