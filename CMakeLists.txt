cmake_minimum_required(VERSION 3.22)

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

project(devector)

# Check if we are in a three build
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
	message(FATAL_ERROR "Prevented in-three build. Please, create a build folder outside of the source code.")
endif()

find_package(PkgConfig REQUIRED)
include(FetchContent)
find_package(X11 REQUIRED)

# Defining vars
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(DEVECTOR_DIR ${SRC_DIR}/main_imgui)
set(3RD_PARTY_DIR ${SRC_DIR}/3rd_party)
set(SDL3_DIR ${3RD_PARTY_DIR}/SDL)
set(IMGUI_DIR ${3RD_PARTY_DIR}/imgui)
set(TINY_DIALOGS_DIR ${3RD_PARTY_DIR}/libtinyfiledialogs)
set(STB_IMAGE_DIR ${SRC_DIR}/stb_image)
set(GLAD_DIR ${SRC_DIR}/glad)
set(CORE_DIR ${SRC_DIR}/core)
set(UTILS_DIR ${SRC_DIR}/utils)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(LUAJIT_DIR ${3RD_PARTY_DIR}/luajit)

# Create 3rd_party folder
file(MAKE_DIRECTORY ${3RD_PARTY_DIR})

# Fetch SDL3 if not already in 3rd_party
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG main
)
FetchContent_MakeAvailable(SDL3)

# Fetch ImGui if not already in 3rd_party
if(NOT EXISTS ${IMGUI_DIR})
	message(STATUS "Cloning ImGui into ${IMGUI_DIR}")
	FetchContent_Declare(
		ImGui
		GIT_REPOSITORY https://github.com/ocornut/imgui.git
		GIT_TAG docking  # Switch to the 'docking' branch
		SOURCE_DIR ${IMGUI_DIR}  # the destination directory
	)
	FetchContent_MakeAvailable(ImGui)
endif()

# Fetch libtinyfiledialogs if not already in 3rd_party
if(NOT EXISTS ${TINY_DIALOGS_DIR})
	message(STATUS "Cloning libtinyfiledialogs into ${TINY_DIALOGS_DIR}")
	FetchContent_Declare(
		libtinyfiledialogs
		GIT_REPOSITORY https://github.com/native-toolkit/libtinyfiledialogs.git
		GIT_TAG master
		SOURCE_DIR ${TINY_DIALOGS_DIR}  # the destination directory
	)
	FetchContent_MakeAvailable(libtinyfiledialogs)
endif()

# Fetch LuaJIT if not already in 3rd_party
if(NOT EXISTS ${LUAJIT_DIR})
	message(STATUS "Cloning LuaJIT into ${LUAJIT_DIR}")
	FetchContent_Declare(
		LuaJIT
		GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
		GIT_TAG v2.1.0-beta3  # Stable version
		SOURCE_DIR ${LUAJIT_DIR}
	)
	FetchContent_MakeAvailable(LuaJIT)
endif()

# Build LuaJIT statically as an external project
include(ExternalProject)
if(WIN32)
	if(MINGW)
		# MinGW on Windows: Use Makefile with static build
		ExternalProject_Add(
			luajit_build
			SOURCE_DIR ${LUAJIT_DIR}
			CONFIGURE_COMMAND ""  # LuaJIT uses its own build system
			BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${LUAJIT_DIR} mingw32-make BUILDMODE=static "TARGET_CFLAGS=-m64"
			INSTALL_COMMAND ""    # No install step; we use the output directly
			BUILD_IN_SOURCE 1     # LuaJIT builds in its source dir
		)
		set(LUAJIT_STATIC_LIB "${LUAJIT_DIR}/src/libluajit.a")
	else()
		# MSVC on Windows: Use msvcbuild.bat
		ExternalProject_Add(
			luajit_build
			SOURCE_DIR ${LUAJIT_DIR}
			CONFIGURE_COMMAND ""  # LuaJIT uses its own build system
			BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${LUAJIT_DIR}/src msvcbuild.bat static "TARGET_CFLAGS=-m64"
			INSTALL_COMMAND ""    # No install step; we use the output directly
			BUILD_IN_SOURCE 1     # LuaJIT builds in its source dir
		)
		set(LUAJIT_STATIC_LIB ${LUAJIT_DIR}/src/libluajit.lib)
	endif()
else()
	# Unix-like: Use Makefile with static build
	ExternalProject_Add(
		luajit_build
		SOURCE_DIR ${LUAJIT_DIR}
		CONFIGURE_COMMAND ""  # LuaJIT uses its own build system
		BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${LUAJIT_DIR} make BUILDMODE=static "TARGET_CFLAGS=-m64"
		INSTALL_COMMAND ""    # No install step; we use the output directly
		BUILD_IN_SOURCE 1     # LuaJIT builds in its source dir
	)
	set(LUAJIT_STATIC_LIB "${LUAJIT_DIR}/src/libluajit.a")
endif()

# Define LuaJIT as an imported static library
add_library(luajit STATIC IMPORTED)
set_target_properties(luajit PROPERTIES
	IMPORTED_LOCATION ${LUAJIT_STATIC_LIB}
	INTERFACE_INCLUDE_DIRECTORIES ${LUAJIT_DIR}/src
)
add_dependencies(luajit luajit_build)

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

# libtinyfiledialogs source
set(TINY_DIALOGS_SRC
	${TINY_DIALOGS_DIR}/tinyfiledialogs.c
)

# stb_image source
set(STB_IMAGE_SRC
	${STB_IMAGE_DIR}/stb_image.h
)

# devector source
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
	${DEVECTOR_DIR}/main/ui/mem_display_window.cpp
	${DEVECTOR_DIR}/main/ui/mem_display_window.h
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

	${DEVECTOR_DIR}/utils/imgui_utils.cpp
	${DEVECTOR_DIR}/utils/imgui_utils.h
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SRC} ${TINY_DIALOGS_SRC} ${STB_IMAGE_SRC})

# Add Unicode definitions for Windows
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GTK)

# Set C++20 as the standard
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

# Include dirs
target_include_directories(${PROJECT_NAME} PRIVATE
	${IMGUI_DIR}
	${IMGUI_DIR}/backends
	${SRC_DIR}
	${GLAD_DIR}/include
	${GLAD_DIR}/src
	${IMGUI_DIR}/misc/cpp
	${LUAJIT_DIR}/src
	${LUAJIT_DIR}/src/host
	${LUAJIT_DIR}/src/jit
	${DEVECTOR_DIR}
	${DEVECTOR_DIR}/main
	${DEVECTOR_DIR}/main/ui
)
if(UNIX AND NOT APPLE)
	# Include GTK and GLib headers
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
