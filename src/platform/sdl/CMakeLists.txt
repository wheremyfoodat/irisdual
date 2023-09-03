
cmake_minimum_required(VERSION 3.2)

project(dual-sdl CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/application.cpp
  src/emulator_thread.cpp
  src/frame_limiter.cpp
  src/main.cpp
)

set(HEADERS
  src/application.hpp
  src/emulator_thread.hpp
  src/frame_limiter.hpp
)

find_package(SDL2 REQUIRED)

add_executable(dual-sdl ${SOURCES} ${HEADERS})
target_include_directories(dual-sdl PRIVATE src)
target_link_libraries(dual-sdl PRIVATE dual SDL2::SDL2)