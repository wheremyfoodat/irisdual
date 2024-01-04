
#pragma once

#include <atom/integer.hpp>
#include <array>

namespace dual::nds::arm7 {

  class WIFI {
    public:
      void Reset();

      u32   Read_IO(u32 address);
      void Write_IO(u32 address, u32 value, u32 mask);

    private:
      std::array<u32, 0x10BE> m_io{};   //< WiFi RAM and stubbed IO registers
      std::array<u8, 0x69> m_bb_regs{}; //< Baseband (BB) internal registers
  };

} // namespace dual::nds::arm7
