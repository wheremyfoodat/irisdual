cmake_minimum_required(VERSION 3.2)
project(dual CXX)

option(PLATFORM_SDL "Build SDL frontend" ON)

find_package(PkgConfig REQUIRED)
option(BUILD_STATIC "Build a statically linked executable" OFF)
if(BUILD_STATIC)
  list(APPEND PKG_CONFIG_EXECUTABLE "--static")
endif()

add_subdirectory(external)
add_subdirectory(src/dual)

if(PLATFORM_SDL)
  add_subdirectory(src/platform/sdl ${CMAKE_CURRENT_BINARY_DIR}/bin/sdl/)
  set_target_properties(dual-sdl PROPERTIES OUTPUT_NAME "dual")
endif()
