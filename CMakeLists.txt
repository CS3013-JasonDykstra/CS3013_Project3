cmake_minimum_required(VERSION 3.16)
project(starting_materials C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(starting_materials
        goatmalloc.c
        goatmalloc.h
        test_goatmalloc.c)
