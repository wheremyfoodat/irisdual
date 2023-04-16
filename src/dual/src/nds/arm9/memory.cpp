
#include <SDL.h>

#include <dual/nds/arm9/memory.hpp>

namespace dual::nds::arm9 {

  namespace bit = atom::bit;

  MemoryBus::MemoryBus(SystemMemory& memory, HW const& hw)
      : m_boot_rom{memory.arm9.bios.data()}
      , m_ewram{memory.ewram.data()}
      , m_lcdc_vram_hack{memory.lcdc_vram_hack.data()}
      , m_io{hw} {
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
        // @todo: remove hacky IO stubs

        // Hah, armwrestler. Such a gullible fool :p
        if(address == 0x04000004) {
          static int vblank = 0;
          static int counter = 0;
          if(++counter == 100000) {
            vblank ^= 1;
            counter = 0;
          }
          return vblank;
        }

        if(address == 0x04000130) {
          const u8* key_state = SDL_GetKeyboardState(nullptr);

          u16 keyinput = 0x03FFu;

          if(key_state[SDL_SCANCODE_A]) keyinput &= ~1u;
          if(key_state[SDL_SCANCODE_S]) keyinput &= ~2u;
          if(key_state[SDL_SCANCODE_BACKSPACE]) keyinput &= ~4u;
          if(key_state[SDL_SCANCODE_RETURN]) keyinput &= ~8u;
          if(key_state[SDL_SCANCODE_RIGHT]) keyinput &= ~16u;
          if(key_state[SDL_SCANCODE_LEFT]) keyinput &= ~32u;
          if(key_state[SDL_SCANCODE_UP]) keyinput &= ~64u;
          if(key_state[SDL_SCANCODE_DOWN]) keyinput &= ~128u;
          if(key_state[SDL_SCANCODE_F]) keyinput &= ~256u;
          if(key_state[SDL_SCANCODE_D]) keyinput &= ~512u;

          return keyinput;
        }

        if constexpr(std::is_same_v<T, u8 >) return m_io.ReadByte(address);
        if constexpr(std::is_same_v<T, u16>) return m_io.ReadHalf(address);
        if constexpr(std::is_same_v<T, u32>) return m_io.ReadWord(address);

        return 0u;
      }
      case 0x06: {
        if(address >= 0x06800000 && address < 0x06818000) {
          return atom::read<T>(m_lcdc_vram_hack, address - 0x06800000u);
        }
        return 0u;
      }
      case 0xFF: {
        if(address >= 0xFFFF0000u) {
          return atom::read<T>(m_boot_rom, address & 0x7FFFu);
        }
        return 0u;
      }
    }

    ATOM_PANIC("arm9: unhandled {}-bit read from 0x{:08X}", bit::number_of_bits<T>(), address);
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
        if constexpr(std::is_same_v<T, u8 >) m_io.WriteByte(address, value);
        if constexpr(std::is_same_v<T, u16>) m_io.WriteHalf(address, value);
        if constexpr(std::is_same_v<T, u32>) m_io.WriteWord(address, value);
        break;
      }
      case 0x06: {
        if(address >= 0x06800000 && address < 0x06818000) {
          atom::write<T>(m_lcdc_vram_hack, address - 0x06800000, value);
        }
        break;
      }
      default: {
        if(address == 0x08005500 && value == 0x08005500) {
          break; // weird rockwrestler write
        }

        ATOM_PANIC("arm9: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
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
