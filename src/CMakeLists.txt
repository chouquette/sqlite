project(medialibrary)
add_definitions("-std=c++11")
add_definitions("-g")
cmake_minimum_required(VERSION 2.8)
list(APPEND SRC_LIST
    sqlite/Table.cpp
)

add_library(MediaLibrary SHARED ${SRC_LIST})
