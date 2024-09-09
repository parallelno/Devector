cmake_minimum_required(VERSION 3.22)

project(devector)

# Check if we are in a three build
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-three build. Please, create a build folder outside of the source code.")
endif()

# Defining vars
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/projects/devector/src)
set(3RD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared_src/3rd_party)
set(SDL3_DIR ${3RD_PARTY_DIR}/SDL)
set(IMGUI_DIR ${3RD_PARTY_DIR}/imgui)
set(GLAD_DIR ${3RD_PARTY_DIR}/glad)
set(SHARED_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shared_src)
set(CORE_DIR ${SHARED_SRC}/core)
set(UTILS_DIR ${SHARED_SRC}/utils)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Create 3rd_party folder
file(MAKE_DIRECTORY ${3RD_PARTY_DIR})

find_package(PkgConfig REQUIRED)

include(FetchContent)
# Fetch SDL3 if not already in 3rd_party
if(NOT EXISTS ${SDL3_DIR})
    message(STATUS "Cloning SDL3 into ${SDL3_DIR}")
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
        SOURCE_DIR ${SDL3_DIR}  # Specify the destination directory
    )
    FetchContent_MakeAvailable(SDL3)
endif()
add_subdirectory(${SDL3_DIR} EXCLUDE_FROM_ALL)

# Fetch ImGui if not already in 3rd_party
if(NOT EXISTS ${IMGUI_DIR})
    message(STATUS "Cloning ImGui into ${IMGUI_DIR}")
    FetchContent_Declare(
        ImGui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking  # Switch to the 'docking' branch
        SOURCE_DIR ${IMGUI_DIR}  # Specify the destination directory
    )
    FetchContent_MakeAvailable(ImGui)
endif()

# Add GLAD
add_library(glad ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# Optionally, build GLEW utilities (glewinfo, visualinfo)
set(BUILD_UTILS OFF CACHE BOOL "Build GLEW utilities")

# ImGui source
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

# devector source
file(GLOB_RECURSE CORE_SRC ${CORE_DIR}/*.cpp ${CORE_DIR}/*.h)
file(GLOB_RECURSE UTILS_SRC ${UTILS_DIR}/*.cpp ${UTILS_DIR}/*.h)

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${CORE_SRC}
    ${UTILS_SRC}
    ${3RD_PARTY_DIR}/njson/json.hpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SRC})

# Add Unicode definitions for Windows
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

# Set C++20 as the standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS OFF)  # Ensures no compiler-specific extensions are used

# Installing and linking GTK3 for open/save dialogs
if(UNIX AND NOT APPLE)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GTK3_LIBRARIES})
endif()

# Linking
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE GL dl)
endif()

# Include dirs
target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SRC_DIR}
    ${SHARED_SRC}
    ${3RD_PARTY_DIR}
    ${GLAD_DIR}/include
    ${GLAD_DIR}/src
    ${IMGUI_DIR}/misc/cpp/
)
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
# 
# Post build commands
# 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

# Create bin directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "--------- Deleting ${BIN_DIR} folder"
)

# Copy executable to bin folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "--------- Executable copied to: ${BIN_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
)

# Copy SDL3 library to bin folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SDL3::SDL3> ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "--------- SDL3 library copied to: ${BIN_DIR}/$<TARGET_FILE_NAME:SDL3::SDL3>"
)

# Copy contents of resources folder directly into bin folder
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DATA_DIR} ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "--------- Data files copied to: ${BIN_DIR}"
)

# Add custom target to run the executable
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E chdir ${BIN_DIR} ./$<TARGET_FILE_NAME:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "--------- Running ${PROJECT_NAME} from ${BIN_DIR}"
)