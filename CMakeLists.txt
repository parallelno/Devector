cmake_minimum_required(VERSION 3.22)

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

project(devector VERSION 9.3.2)

# Prevent in-source build
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
	message(FATAL_ERROR "Prevented in-source build. Please create a build folder outside the source code.")
endif()

find_package(PkgConfig REQUIRED)
include(FetchContent)
find_package(X11 REQUIRED)

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(DEVECTOR_DIR ${SRC_DIR}/main_imgui)
set(3RD_PARTY_DIR ${SRC_DIR}/3rd_party)
set(STB_IMAGE_DIR ${SRC_DIR}/stb_image)
set(GLAD_DIR ${SRC_DIR}/glad)
set(CORE_DIR ${SRC_DIR}/core)
set(UTILS_DIR ${SRC_DIR}/utils)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# SDL3: FetchContent, default build dir
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.20
)
FetchContent_MakeAvailable(SDL3)

# ImGui: FetchContent without SOURCE_DIR to let CMake manage it
FetchContent_Declare(
  ImGui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.2b-docking
)
FetchContent_MakeAvailable(ImGui)

# libtinyfiledialogs
FetchContent_Declare(
  libtinyfiledialogs
  GIT_REPOSITORY https://github.com/native-toolkit/libtinyfiledialogs.git
  GIT_TAG 2.9.3
)
FetchContent_MakeAvailable(libtinyfiledialogs)

# LuaJIT
FetchContent_Declare(
  LuaJIT
  GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
  GIT_TAG v2.1.0-beta3
)
FetchContent_MakeAvailable(LuaJIT)
FetchContent_GetProperties(LuaJIT)

# ExternalProject for LuaJIT static build (same as before)
include(ExternalProject)

if(WIN32)
	if(MINGW)
		ExternalProject_Add(
			luajit_build
			SOURCE_DIR ${luajit_SOURCE_DIR}
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${luajit_SOURCE_DIR} mingw32-make BUILDMODE=static "TARGET_CFLAGS=-m64"
			INSTALL_COMMAND ""
			BUILD_IN_SOURCE 1
		)
	else()
		ExternalProject_Add(
			luajit_build
			SOURCE_DIR ${luajit_SOURCE_DIR}
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${luajit_SOURCE_DIR}/src msvcbuild.bat static "TARGET_CFLAGS=-m64"
			INSTALL_COMMAND ""
			BUILD_IN_SOURCE 1
		)
	endif()
else()
	ExternalProject_Add(
		luajit_build
		SOURCE_DIR ${luajit_SOURCE_DIR}
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${luajit_SOURCE_DIR} make BUILDMODE=static "TARGET_CFLAGS=-m64"
		INSTALL_COMMAND ""
		BUILD_IN_SOURCE 1
	)
endif()

# Set LuaJIT static lib path accordingly
if(WIN32)
	if(MINGW)
		set(LUAJIT_STATIC_LIB "${luajit_SOURCE_DIR}/src/libluajit.a")
	else()
		set(LUAJIT_STATIC_LIB "${luajit_SOURCE_DIR}/src/libluajit.lib")
	endif()
else()
	set(LUAJIT_STATIC_LIB "${luajit_SOURCE_DIR}/src/libluajit.a")
endif()

# Add LuaJIT as imported static lib
add_library(luajit STATIC IMPORTED)
set_target_properties(luajit PROPERTIES
	IMPORTED_LOCATION ${LUAJIT_STATIC_LIB}
	INTERFACE_INCLUDE_DIRECTORIES ${luajit_SOURCE_DIR}/src
)

# Add GLAD
add_library(glad ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# Optionally, build GLEW utilities (glewinfo, visualinfo)
set(BUILD_UTILS OFF CACHE BOOL "Build GLEW utilities")

# ImGui source files (using imgui_SOURCE_DIR from FetchContent)
FetchContent_GetProperties(ImGui)
set(IMGUI_SRC
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
	${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

# libtinyfiledialogs source files
FetchContent_GetProperties(libtinyfiledialogs)
set(TINY_DIALOGS_SRC
	${libtinyfiledialogs_SOURCE_DIR}/tinyfiledialogs.c
)

# stb_image source
set(STB_IMAGE_SRC
	${STB_IMAGE_DIR}/stb_image.h
)

# ----- Version header (generated into build dir) -----
set(GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_INCLUDE_DIR}")

configure_file(
    "${DEVECTOR_DIR}/version.h.in"
    "${GENERATED_INCLUDE_DIR}/version.h"
    @ONLY
)

# devector source files
file(GLOB_RECURSE CORE_SRC ${CORE_DIR}/*.cpp ${CORE_DIR}/*.h)
file(GLOB_RECURSE UTILS_SRC ${UTILS_DIR}/*.cpp ${UTILS_DIR}/*.h)

set(SOURCES
	${CORE_SRC}
	${UTILS_SRC}
	${SRC_DIR}/njson/json.hpp
	# devector
	${DEVECTOR_DIR}/main/main.cpp
	${DEVECTOR_DIR}/main/devector_app.h
	${DEVECTOR_DIR}/main/devector_app.cpp

	${DEVECTOR_DIR}/main/imgui_app.h
	${DEVECTOR_DIR}/main/imgui_app.cpp

	${DEVECTOR_DIR}/main/scheduler.h
	${DEVECTOR_DIR}/main/scheduler.cpp
	${DEVECTOR_DIR}/main/signals.h

	${DEVECTOR_DIR}/main/ui/about_window.cpp
	${DEVECTOR_DIR}/main/ui/about_window.h
	${DEVECTOR_DIR}/main/ui/base_window.cpp
	${DEVECTOR_DIR}/main/ui/base_window.h
	${DEVECTOR_DIR}/main/ui/breakpoints_window.cpp
	${DEVECTOR_DIR}/main/ui/breakpoints_window.h
	${DEVECTOR_DIR}/main/ui/disasm_window.cpp
	${DEVECTOR_DIR}/main/ui/disasm_window.h
	${DEVECTOR_DIR}/main/ui/display_window.cpp
	${DEVECTOR_DIR}/main/ui/display_window.h
	${DEVECTOR_DIR}/main/ui/feedback_window.cpp
	${DEVECTOR_DIR}/main/ui/feedback_window.h
	${DEVECTOR_DIR}/main/ui/hardware_stats_window.cpp
	${DEVECTOR_DIR}/main/ui/hardware_stats_window.h
	${DEVECTOR_DIR}/main/ui/hex_viewer_window.cpp
	${DEVECTOR_DIR}/main/ui/hex_viewer_window.h
	${DEVECTOR_DIR}/main/ui/hex_viewer_window_consts.h
	${DEVECTOR_DIR}/main/ui/mem_display_window.cpp
	${DEVECTOR_DIR}/main/ui/mem_display_window.h
	${DEVECTOR_DIR}/main/ui/mem_display_window_consts.h
	${DEVECTOR_DIR}/main/ui/recorder_window.cpp
	${DEVECTOR_DIR}/main/ui/recorder_window.h
	${DEVECTOR_DIR}/main/ui/trace_log_window.cpp
	${DEVECTOR_DIR}/main/ui/trace_log_window.h
	${DEVECTOR_DIR}/main/ui/watchpoints_window.cpp
	${DEVECTOR_DIR}/main/ui/watchpoints_window.h
	${DEVECTOR_DIR}/main/ui/keyboard_window.cpp
	${DEVECTOR_DIR}/main/ui/keyboard_window.h
	${DEVECTOR_DIR}/main/ui/search_window.cpp
	${DEVECTOR_DIR}/main/ui/search_window.h
	${DEVECTOR_DIR}/main/ui/debugdata_window.cpp
	${DEVECTOR_DIR}/main/ui/debugdata_window.h
	${DEVECTOR_DIR}/main/ui/label_edit_modal.h
	${DEVECTOR_DIR}/main/ui/label_edit_modal.cpp
	${DEVECTOR_DIR}/main/ui/const_edit_modal.h
	${DEVECTOR_DIR}/main/ui/const_edit_modal.cpp
	${DEVECTOR_DIR}/main/ui/comment_edit_modal.h
	${DEVECTOR_DIR}/main/ui/comment_edit_modal.cpp
	${DEVECTOR_DIR}/main/ui/memory_edit_modal.h
	${DEVECTOR_DIR}/main/ui/memory_edit_modal.cpp
	${DEVECTOR_DIR}/main/ui/code_perf_edit_modal.h
	${DEVECTOR_DIR}/main/ui/code_perf_edit_modal.cpp
	${DEVECTOR_DIR}/main/ui/script_edit_modal.h
	${DEVECTOR_DIR}/main/ui/script_edit_modal.cpp
	${DEVECTOR_DIR}/main/ui/trace_log_popup.h
	${DEVECTOR_DIR}/main/ui/trace_log_popup.cpp
	${DEVECTOR_DIR}/main/ui/breakpoints_popup.h
	${DEVECTOR_DIR}/main/ui/breakpoints_popup.cpp
	${DEVECTOR_DIR}/main/ui/watchpoints_popup.h
	${DEVECTOR_DIR}/main/ui/watchpoints_popup.cpp
	${DEVECTOR_DIR}/main/ui/disasm_popup.h
	${DEVECTOR_DIR}/main/ui/disasm_popup.cpp
	${DEVECTOR_DIR}/main/ui/debugdata_popup.h
	${DEVECTOR_DIR}/main/ui/debugdata_popup.cpp

	${DEVECTOR_DIR}/utils/imgui_utils.h
	${DEVECTOR_DIR}/utils/imgui_utils.cpp

	${GENERATED_INCLUDE_DIR}/version.h
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SRC} ${TINY_DIALOGS_SRC} ${STB_IMAGE_SRC})

# Unicode definitions for Windows
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GTK)

# C++20 standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS OFF)  # Ensures no compiler-specific extensions are used

# Linking

# Force static linking of libgcc and libstdc++ to avoid runtime dependency issues
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE GL dl)
	target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE luajit)
if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE user32)  # LuaJIT may require this on Windows
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
	${imgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}/backends
	${SRC_DIR}
	${GLAD_DIR}/include
	${GLAD_DIR}/src
	${imgui_SOURCE_DIR}/misc/cpp
	${luajit_SOURCE_DIR}/src
	${luajit_SOURCE_DIR}/src/host
	${luajit_SOURCE_DIR}/src/jit
	${libtinyfiledialogs_SOURCE_DIR}
	${DEVECTOR_DIR}
	${DEVECTOR_DIR}/main
	${DEVECTOR_DIR}/main/ui
	${GENERATED_INCLUDE_DIR}
)

if(UNIX AND NOT APPLE)
	target_include_directories(${PROJECT_NAME} PRIVATE
		${GTK_DIR}/include
		${GLIB_DIR}/include
	)
endif()

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
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR} ${BIN_DIR}
	COMMAND ${CMAKE_COMMAND} -E echo "--------- Data files copied to: ${BIN_DIR}"
)

# Add custom target to run the executable
add_custom_target(run
	COMMAND ${CMAKE_COMMAND} -E chdir ${BIN_DIR} ./$<TARGET_FILE_NAME:${PROJECT_NAME}>
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${BIN_DIR}
	COMMAND ${CMAKE_COMMAND} -E echo "--------- Running ${PROJECT_NAME} from ${BIN_DIR}"
)
