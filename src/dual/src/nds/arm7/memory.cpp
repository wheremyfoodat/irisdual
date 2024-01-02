
#include <SDL.h>

#include <dual/nds/arm7/memory.hpp>

namespace dual::nds::arm7 {

  namespace bit = atom::bit;

  MemoryBus::MemoryBus(SystemMemory& memory, const HW& hw)
      : m_boot_rom{memory.arm7.bios.data()}
      , m_ewram{memory.ewram.data()}
      , m_iwram{memory.arm7.iwram.data()}
      , m_swram{memory.swram}
      , m_vram{memory.vram}
      , m_io{hw} {
  }

  void MemoryBus::Reset() {
    m_io.postflg = 0u;
  }

  template<typename T> T MemoryBus::Read(u32 address, Bus bus) {
    address &= ~(sizeof(T) - 1u);

    switch(address >> 24) {
      case 0x00: {
        return atom::read<T>(m_boot_rom, address & 0x3FFFu);
      }
      case 0x02: {
        return atom::read<T>(m_ewram, address & 0x3FFFFFu);
      }
      case 0x03: {
        if((address & 0x00800000u) || !m_swram.arm7.data) {
          return atom::read<T>(m_iwram, address & 0xFFFFu);
        }
        return atom::read<T>(m_swram.arm7.data, address & m_swram.arm7.mask);
      }
      case 0x04: {
        if(address == 0x04000136) {
          const u8* key_state = SDL_GetKeyboardState(nullptr);

          u16 extkeyinput = 0x3Fu;

          if(key_state[SDL_SCANCODE_Q]) extkeyinput &= ~1u;
          if(key_state[SDL_SCANCODE_W]) extkeyinput &= ~2u;

          return extkeyinput;
        }

        if constexpr(std::is_same_v<T, u8 >) return m_io.ReadByte(address);
        if constexpr(std::is_same_v<T, u16>) return m_io.ReadHalf(address);
        if constexpr(std::is_same_v<T, u32>) return m_io.ReadWord(address);

        return 0;
      }
      case 0x06: {
        return m_vram.region_arm7_wram.Read<T>(address);
      }
    }

    //ATOM_PANIC("arm7: unhandled {}-bit read from 0x{:08X}", bit::number_of_bits<T>(), address);
    return 0;
  }

  template<typename T> void MemoryBus::Write(u32 address, T value, Bus bus) {
    address &= ~(sizeof(T) - 1u);

    switch(address >> 24) {
      case 0x02: {
        atom::write<T>(m_ewram, address & 0x3FFFFFu, value);
        break;
      }
      case 0x03: {
        if((address & 0x00800000u) || !m_swram.arm7.data) {
          atom::write<T>(m_iwram, address & 0xFFFFu, value);
        } else {
          atom::write<T>(m_swram.arm7.data, address & m_swram.arm7.mask, value);
        }
        break;
      }
      case 0x04: {
        if constexpr(std::is_same_v<T, u8 >) m_io.WriteByte(address, value);
        if constexpr(std::is_same_v<T, u16>) m_io.WriteHalf(address, value);
        if constexpr(std::is_same_v<T, u32>) m_io.WriteWord(address, value);
        break;
      }
      case 0x06: {
        m_vram.region_arm7_wram.Write<T>(address, value);
        break;
      }
      default: {
        if(address == 0x08005500 && value == 0x08005500) {
          break; // weird rockwrestler write
        }

        //ATOM_PANIC("arm7: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
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
