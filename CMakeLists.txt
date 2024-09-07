cmake_minimum_required(VERSION 3.22)

project(devector)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-three build. Please, create a build folder outside of the source code.")
endif()

find_package(PkgConfig REQUIRED)

find_package(SDL3)
if (SDL3_FOUND)
    message(STATUS "SDL3 found!")
    set(SDL3_DIR="../SDL/")
else()
    message(STATUS "SDL3 is not installed. Using locally download SDL repo")
    add_subdirectory(/home/parallelno/work/devector_cross/shared_src/3rd_party/SDL EXCLUDE_FROM_ALL)
endif()

set(SRC_DIR /home/parallelno/work/devector_cross/projects/devector/src)

add_library(my_lib SHARED ${SRC_DIR}/my_lib.cpp)

add_executable(devector ${SRC_DIR}/main.cpp)

target_link_libraries(devector PRIVATE my_lib SDL3::SDL3)
