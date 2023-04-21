
#pragma once

#include <array>
#include <atom/integer.hpp>
#include <dual/nds/vram/vram.hpp>
#include <dual/nds/swram.hpp>

namespace dual::nds {

  struct SystemMemory {
    std::array<u8, 0x400000> ewram{};
    std::array<u8, 0x18000> lcdc_vram_hack{};

    SWRAM swram{};

    VRAM vram{};
    std::array<u8, 0x800> pram;
    std::array<u8, 0x800> oam;

    struct ARM9 {
      std::array<u8, 0x8000> bios{};
      std::array<u8, 0x4000> dtcm{};
      std::array<u8, 0x8000> itcm{};
    } arm9{};

    struct ARM7 {
      std::array<u8, 0x4000> bios{};
      std::array<u8, 0x10000> iwram{};
    } arm7{};
  };

} // namespace dual::nds
