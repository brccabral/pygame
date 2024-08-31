cmake_minimum_required(VERSION 3.29)
project(pygame_examples C)

set(CMAKE_C_STANDARD 11)

add_executable(basic_window basic_window.c)
target_link_libraries(basic_window PUBLIC pygame)
