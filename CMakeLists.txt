cmake_minimum_required (VERSION 2.6)
project (PLASMA)
string(TOLOWER ${PROJECT_NAME} EXEC_NAME)
set (VERSION_MAJOR 0)
set (VERSION_MINOR 10)

file(GLOB ${PROJECT_NAME}_SRCS RELATIVE ${PROJECT_SOURCE_DIR} *.c)

find_package (PkgConfig)
pkg_check_modules (SDL2 sdl2)
include_directories(${SDL2_INCLUDE_DIRS})



add_executable (${EXEC_NAME} ${${PROJECT_NAME}_SRCS})
target_link_libraries (${EXEC_NAME} ${SDL2_LIBRARIES} m)



# vim:set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab :
