cmake_minimum_required(VERSION 3.2)
project(dual-core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/arm/tablegen/tablegen.cpp
  src/arm/arm.cpp
  src/common/scheduler.cpp
  src/nds/arm7/memory.cpp
  src/nds/arm9/cp15.cpp
  src/nds/arm9/io.cpp
  src/nds/arm9/memory.cpp
  src/nds/nds.cpp
)

set(HEADERS
  src/arm/handlers/arithmetic.inl
  src/arm/handlers/handler16.inl
  src/arm/handlers/handler32.inl
  src/arm/handlers/memory.inl
  src/arm/tablegen/decoder.hpp
  src/arm/tablegen/gen_arm.hpp
  src/arm/tablegen/gen_thumb.hpp
  src/arm/arm.hpp
)

set(HEADERS_PUBLIC
  include/dual/arm/coprocessor.hpp
  include/dual/arm/cpu.hpp
  include/dual/arm/memory.hpp
  include/dual/common/scheduler.hpp
  include/dual/nds/arm7/memory.hpp
  include/dual/nds/arm9/cp15.hpp
  include/dual/nds/arm9/memory.hpp
  include/dual/nds/header.hpp
  include/dual/nds/nds.hpp
  include/dual/nds/rom.hpp
  include/dual/nds/system_memory.hpp
)

add_library(dual ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(dual PUBLIC atom-common atom-logger)

target_include_directories(dual PUBLIC include)
target_include_directories(dual PRIVATE src)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(dual PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fbracket-depth=8192>)
endif()

# @todo: remove this when properly implementing key input
find_package(SDL2 REQUIRED)
target_link_libraries(dual PRIVATE SDL2::SDL2)