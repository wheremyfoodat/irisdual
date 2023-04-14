
#pragma once

#include <array>
#include <atom/integer.hpp>

namespace dual::nds {

  struct SystemMemory {
    std::array<u8, 0x400000> ewram{};

    struct ARM9 {
      std::array<u8, 0x8000> bios{};
      std::array<u8, 0x4000> dtcm{};
      std::array<u8, 0x8000> itcm{};
    } arm9{};
  };

} // namespace dual::nds
