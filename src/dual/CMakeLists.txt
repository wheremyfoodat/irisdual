cmake_minimum_required(VERSION 3.2)
project(dual-core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/arm/tablegen/tablegen.cpp
  src/arm/arm.cpp
  src/common/scheduler.cpp
  src/nds/arm7/dma.cpp
  src/nds/arm7/io.cpp
  src/nds/arm7/memory.cpp
  src/nds/arm7/spi.cpp
  src/nds/arm9/cp15.cpp
  src/nds/arm9/dma.cpp
  src/nds/arm9/io.cpp
  src/nds/arm9/math.cpp
  src/nds/arm9/memory.cpp
  src/nds/backup/eeprom.cpp
  src/nds/backup/eeprom512b.cpp
  src/nds/backup/flash.cpp
  src/nds/video_unit/ppu/render/affine.cpp
  src/nds/video_unit/ppu/render/oam.cpp
  src/nds/video_unit/ppu/render/text.cpp
  src/nds/video_unit/ppu/render/window.cpp
  src/nds/video_unit/ppu/composer.cpp
  src/nds/video_unit/ppu/ppu.cpp
  src/nds/video_unit/ppu/registers.cpp
  src/nds/video_unit/video_unit.cpp
  src/nds/cartridge.cpp
  src/nds/ipc.cpp
  src/nds/irq.cpp
  src/nds/nds.cpp
  src/nds/swram.cpp
  src/nds/timer.cpp
  src/nds/vram.cpp
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
  include/dual/common/backup_file.hpp
  include/dual/common/fifo.hpp
  include/dual/common/scheduler.hpp
  include/dual/nds/arm7/dma.hpp
  include/dual/nds/arm7/memory.hpp
  include/dual/nds/arm7/spi.hpp
  include/dual/nds/arm9/cp15.hpp
  include/dual/nds/arm9/dma.hpp
  include/dual/nds/arm9/math.hpp
  include/dual/nds/arm9/memory.hpp
  include/dual/nds/backup/eeprom.hpp
  include/dual/nds/backup/eeprom512b.hpp
  include/dual/nds/backup/flash.hpp
  include/dual/nds/video_unit/ppu/ppu.hpp
  include/dual/nds/video_unit/ppu/registers.hpp
  include/dual/nds/video_unit/video_unit.hpp
  include/dual/nds/vram/region.hpp
  include/dual/nds/vram/vram.hpp
  include/dual/nds/cartridge.hpp
  include/dual/nds/header.hpp
  include/dual/nds/nds.hpp
  include/dual/nds/rom.hpp
  include/dual/nds/swram.hpp
  include/dual/nds/system_memory.hpp
  include/dual/nds/timer.hpp
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