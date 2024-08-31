cmake_minimum_required(VERSION 3.29)
project(pygame C)

set(CMAKE_C_STANDARD 11)

add_library(pygame STATIC
        src/base.c
        src/display.c
        src/event.c
        src/surface.c
        src/surface_fill.c
        src/surflock.c
        src/color.c
)
target_include_directories(pygame PUBLIC /usr/include/SDL2)
target_include_directories(pygame PUBLIC include)
target_include_directories(pygame PRIVATE src)
target_link_libraries(pygame PUBLIC SDL2)
