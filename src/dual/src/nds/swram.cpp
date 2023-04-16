
#include <dual/nds/swram.hpp>

namespace dual::nds {

  void SWRAM::Reset() {
    m_swram.fill(0);
    Write_WRAMCNT(3u);
  }

  u32 SWRAM::Read_WRAMCNT() {
    return m_wramcnt;
  }

  void SWRAM::Write_WRAMCNT(u8 value) {
    const u8 allocation = value & 3u;

    if(allocation == m_wramcnt) {
      return;
    }

    switch(allocation) {
      case 0b00:
        arm9 = { &m_swram[0], 0x7FFFu };
        arm7 = { nullptr, 0u };
        break;
      case 0b01:
        arm9 = { &m_swram[0x4000], 0x3FFFu };
        arm7 = { &m_swram[0x0000], 0x3FFFu };
        break;
      case 0b10:
        arm9 = { &m_swram[0x0000], 0x3FFFu };
        arm7 = { &m_swram[0x4000], 0x3FFFu };
        break;
      default:
        arm9 = { nullptr, 0u };
        arm7 = { &m_swram[0], 0x7FFFu };
        break;
    }

    m_wramcnt = allocation;
  }

} // namespace dual::nds