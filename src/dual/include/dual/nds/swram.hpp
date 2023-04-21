
#pragma once

#include <array>
#include <atom/integer.hpp>

namespace dual::nds {

  struct SWRAM {
    void Reset();

    u32   Read_WRAMCNT();
    void Write_WRAMCNT(u8 value);

    struct Allocation {
      u8* data{};
      u32 mask{};
    } arm9{}, arm7{};

  //private:
    std::array<u8, 0x8000> m_swram;

    u8 m_wramcnt = 0u;
  };

} // namespace dual::nds
