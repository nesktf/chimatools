cmake_minimum_required(VERSION 3.25)

link_libraries(chimatools)
include_directories("${CHIMA_DIR}/include")

add_executable(cpp_example cpp_example.cpp)
set_target_properties(cpp_example PROPERTIES CXX_STANDARD 17)

add_executable(c_example c_example.c)
set_target_properties(c_example PROPERTIES C_STANDARD 11)
