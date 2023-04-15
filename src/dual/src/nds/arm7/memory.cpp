
#include <dual/nds/arm7/memory.hpp>

namespace dual::nds::arm7 {

  namespace bit = atom::bit;

  MemoryBus::MemoryBus(SystemMemory& memory)
      : m_ewram{memory.ewram.data()}
      , m_iwram{memory.arm7.iwram.data()} {
  }

  template<typename T> T MemoryBus::Read(u32 address, Bus bus) {
    address &= ~(sizeof(T) - 1u);

    switch(address >> 24) {
      case 0x02: {
        return atom::read<T>(m_ewram, address & 0x3FFFFFu);
      }
      case 0x03: {
        if(address & 0x00800000u) {
          return atom::read<T>(m_iwram, address & 0xFFFFu);
        }
        break;
      }
      case 0x04: {
        ATOM_ERROR("arm7: unhandled {}-bit IO read from 0x{:08X}", bit::number_of_bits<T>(), address);
        return 0;
      }
    }

    ATOM_PANIC("arm7: unhandled {}-bit read from 0x{:08X}", bit::number_of_bits<T>(), address);
  }

  template<typename T> void MemoryBus::Write(u32 address, T value, Bus bus) {
    address &= ~(sizeof(T) - 1u);

    switch(address >> 24) {
      case 0x02: {
        atom::write<T>(m_ewram, address & 0x3FFFFFu, value);
        break;
      }
      case 0x03: {
        if(address & 0x00800000u) {
          atom::write<T>(m_iwram, address & 0xFFFFu, value);
        } else {
          ATOM_PANIC("arm7: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
        }
        break;
      }
      case 0x04: {
        ATOM_ERROR("arm7: unhandled {}-bit IO write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
        break;
      }
      default: {
        ATOM_PANIC("arm7: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
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

} // namespace dual::nds::arm7
