project(medialibrary)
add_definitions("-std=c++11")
add_definitions("-g")
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(Sqlite3 REQUIRED)


list(APPEND SRC_LIST
    sqlite/Table.cpp
)

add_library(MediaLibrary SHARED ${SRC_LIST})
target_link_libraries(MediaLibrary ${SQLITE3_LIBRARIES})
