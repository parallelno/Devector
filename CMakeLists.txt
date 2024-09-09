cmake_minimum_required(VERSION 3.22)

project(devector)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-three build. Please, create a build folder outside of the source code.")
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/projects/devector/src)
set(3RD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared_src/3rd_party)
set(IMGUI_DIR ${3RD_PARTY_DIR}/imgui)
set(SDL3_DIR ${3RD_PARTY_DIR}/SDL)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Create 3rd_party folder
file(MAKE_DIRECTORY ${3RD_PARTY_DIR})

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

add_subdirectory(${SDL3_DIR} EXCLUDE_FROM_ALL)

# ImGui source
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# devector source
set(SOURCES
    ${SRC_DIR}/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SRC_DIR}/ui
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE GL dl)


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

# Copy contents of data folder directly into bin folder
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
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