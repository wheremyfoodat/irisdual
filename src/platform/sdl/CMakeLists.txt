
cmake_minimum_required(VERSION 3.2)

project(dual-sdl CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/application.cpp
  src/emulator_thread.cpp
  src/main.cpp
  src/sdl2_audio_driver.cpp
)

set(HEADERS
  src/application.hpp
  src/emulator_thread.hpp
  src/sdl2_audio_driver.hpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

add_executable(dual-sdl ${SOURCES} ${HEADERS})
target_include_directories(dual-sdl PRIVATE src)
target_link_libraries(dual-sdl PRIVATE dual PkgConfig::SDL2)