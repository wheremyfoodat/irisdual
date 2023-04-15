
#include <dual/nds/arm9/memory.hpp>

namespace dual::nds::arm9 {

  namespace bit = atom::bit;

  MemoryBus::MemoryBus(SystemMemory& memory) : m_ewram{memory.ewram.data()} {
    m_dtcm.data = memory.arm9.dtcm.data();
    m_itcm.data = memory.arm9.itcm.data();
  }

  void MemoryBus::SetupDTCM(TCM::Config const& config) {
    m_dtcm.config = config;
  }

  void MemoryBus::SetupITCM(TCM::Config const& config) {
    m_itcm.config = config;
  }

  template<typename T> T MemoryBus::Read(u32 address, Bus bus) {
    address &= ~(sizeof(T) - 1u);

    if(
      bus != Bus::System && m_itcm.config.readable &&
      address >= m_itcm.config.base_address &&
      address <= m_itcm.config.high_address
    ) {
      return atom::read<T>(m_itcm.data, (address - m_itcm.config.base_address) & 0x7FFFu);
    }

    if(
      bus == Bus::Data && m_dtcm.config.readable &&
      address >= m_dtcm.config.base_address &&
      address <= m_dtcm.config.high_address
    ) {
      return atom::read<T>(m_dtcm.data, (address - m_dtcm.config.base_address) & 0x3FFFu);
    }

    switch(address >> 24) {
      case 0x02: {
        return atom::read<T>(m_ewram, address & 0x3FFFFFu);
      }
      case 0x04: {
        // Hah, armwrestler. Such a gullible fool :p
        if(address == 0x04000004) {
          static int vblank = 0;
          vblank ^= 1;
          return vblank;
        }

        ATOM_ERROR("arm9: unhandled {}-bit IO read from 0x{:08X}", bit::number_of_bits<T>(), address);
        return 0;
      }
    }

    ATOM_PANIC("unhandled {}-bit read from 0x{:08X}", bit::number_of_bits<T>(), address);
  }

  template<typename T> void MemoryBus::Write(u32 address, T value, Bus bus) {
    address &= ~(sizeof(T) - 1u);

    if(
      bus != Bus::System && m_itcm.config.writable &&
      address >= m_itcm.config.base_address &&
      address <= m_itcm.config.high_address
    ) {
      atom::write<T>(m_itcm.data, (address - m_itcm.config.base_address) & 0x7FFFu, value);
      return;
    }

    if(
      bus == Bus::Data && m_dtcm.config.writable &&
      address >= m_dtcm.config.base_address &&
      address <= m_dtcm.config.high_address
    ) {
      atom::write<T>(m_dtcm.data, (address - m_dtcm.config.base_address) & 0x3FFFu, value);
      return;
    }

    switch(address >> 24) {
      case 0x02: {
        atom::write<T>(m_ewram, address & 0x3FFFFFu, value);
        break;
      }
      case 0x04: {
        ATOM_ERROR("arm9: unhandled {}-bit IO write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
        break;
      }
      default: {
        ATOM_PANIC("unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
      }
    }
  }

  u8 MemoryBus::ReadByte(u32 address, Bus bus) {
    return Read<u8>(address, bus);
  }

  u16 MemoryBus::ReadHalf(u32 address, Bus bus) {
    return Read<u16>(address, bus);
  }

  u32 MemoryBus::ReadWord(u32 address, Bus bus) {
    return Read<u32>(address, bus);
  }

  void MemoryBus::WriteByte(u32 address, u8 value, Bus bus) {
    Write<u8>(address, value, bus);
  }

  void MemoryBus::WriteHalf(u32 address, u16 value, Bus bus) {
    Write<u16>(address, value, bus);
  }

  void MemoryBus::WriteWord(u32 address, u32 value, Bus bus) {
    Write<u32>(address, value, bus);
  }

} // namespace dual::nds::arm9
