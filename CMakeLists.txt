cmake_minimum_required (VERSION 2.6)
project (sdl_playground C)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 10)

file(GLOB ${PROJECT_NAME}_SRCS RELATIVE ${PROJECT_SOURCE_DIR} *.c)

find_package (PkgConfig)
pkg_check_modules (SDL2 sdl2)
include_directories(${SDL2_INCLUDE_DIRS})

# This only makes sense for self-contained C files
foreach (sdl_source ${${PROJECT_NAME}_SRCS})
	string (REPLACE ".c" "" sdl_executable ${sdl_source})
	message(STATUS "Compiling ${sdl_source} to ${sdl_executable}")
	add_executable ("${sdl_executable}" "${sdl_source}")
	target_link_libraries ("${sdl_executable}" ${SDL2_LIBRARIES} m)
endforeach (sdl_source ${${PROJECT_NAME}_SRCS})

# vim:set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab :
