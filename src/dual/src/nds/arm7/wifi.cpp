
#include <atom/panic.hpp>
#include <dual/nds/arm7/wifi.hpp>

namespace dual::nds::arm7 {

  void WIFI::Reset() {
    m_io.fill(0u);
    m_bb_regs.fill(0u);
  }

  u32 WIFI::Read_IO(u32 address) {
    // We hardcode W_POWERSTATE to indicate that the WiFi hardware is powered down.
    // This is done to break out of an otherwise infinite loop in the mainline Pokémon games.
    if(address == 0x0480803Cu) {
      return 0x200u;
    }

    const size_t index = (address - 0x04804000u) >> 2;
    if(index >= m_io.size()) {
      ATOM_PANIC("unhandled WiFi IO read from 0x{:08X}", address);
    }
    return m_io[index];
  }

  void WIFI::Write_IO(u32 address, u32 value, u32 mask) {
    const size_t index = (address - 0x04804000u) >> 2;
    if(index >= m_io.size()) {
      ATOM_PANIC("unhandled WiFi IO write to 0x{:08X}", address);
    }
    m_io[index] = (value & mask) | (m_io[index] & ~mask);

    // Handle BB internal register read and write operations
    if(address == 0x04808158u && mask & 0xFF00u) {
      const u32 direction = (value >> 12) & 0xFu;
      const u8  reg = (u8)m_io[0x1056];

      if(reg < m_bb_regs.size()) {
        // 5: latch W_BB_WRITE into BB internal register
        // 6: latch BB internal register into W_BB_READ
        switch(direction) {
          case 5: m_bb_regs[reg] = (u8)(m_io[0x1056] >> 16); break;
          case 6: m_io[0x1057] = m_bb_regs[reg] | (m_io[0x1057] & ~0xFFFFu); break;
        }
      }
    }
  }

} // namespace dual::nds::arm7