
#include <dual/nds/arm9/memory.hpp>

namespace dual::nds::arm9 {

  namespace bit = atom::bit;

  MemoryBus::MemoryBus(SystemMemory& memory, const HW& hw)
      : m_boot_rom{memory.arm9.bios.data()}
      , m_ewram{memory.ewram.data()}
      , m_pram{memory.pram.data()}
      , m_oam{memory.oam.data()}
      , m_swram{hw.swram}
      , m_vram{hw.vram}
      , m_io{hw} {
    m_dtcm.data = memory.arm9.dtcm.data();
    m_itcm.data = memory.arm9.itcm.data();
  }

  void MemoryBus::Reset() {
    m_io.postflg = 0u;
  }

  void MemoryBus::SetupDTCM(const TCM::Config& config) {
    m_dtcm.config = config;
  }

  void MemoryBus::SetupITCM(const TCM::Config& config) {
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
      case 0x03: {
        if(!m_swram.arm9.data) {
          return 0u;
        }
        return atom::read<T>(m_swram.arm9.data, address & m_swram.arm9.mask);
      }
      case 0x04: {
        if constexpr(std::is_same_v<T, u8 >) return m_io.ReadByte(address);
        if constexpr(std::is_same_v<T, u16>) return m_io.ReadHalf(address);
        if constexpr(std::is_same_v<T, u32>) return m_io.ReadWord(address);

        return 0u;
      }
      case 0x05: {
        return ReadPRAM<T>(address);
      }
      case 0x06: {
        switch((address >> 20) & 15) {
          case 0: case 1: return ReadVRAM_PPU_BG <T>(address, 0);
          case 2: case 3: return ReadVRAM_PPU_BG <T>(address, 1);
          case 4: case 5: return ReadVRAM_PPU_OBJ<T>(address, 0);
          case 6: case 7: return ReadVRAM_PPU_OBJ<T>(address, 1);
          default:        return ReadVRAM_LCDC<T>(address);
        }
      }
      case 0x07: {
        return ReadOAM<T>(address);
      }
      case 0xFF: {
        if(address >= 0xFFFF0000u) {
          return atom::read<T>(m_boot_rom, address & 0x7FFFu);
        }
        return 0u;
      }
    }

    // ATOM_PANIC("arm9: unhandled {}-bit read from 0x{:08X}", bit::number_of_bits<T>(), address);
    return 0;
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
      case 0x03: {
        if(!m_swram.arm9.data) {
          return;
        }
        atom::write<T>(m_swram.arm9.data, address & m_swram.arm9.mask, value);
        break;
      }
      case 0x04: {
        if constexpr(std::is_same_v<T, u8 >) m_io.WriteByte(address, value);
        if constexpr(std::is_same_v<T, u16>) m_io.WriteHalf(address, value);
        if constexpr(std::is_same_v<T, u32>) m_io.WriteWord(address, value);
        break;
      }
      case 0x05: {
        if(std::is_same_v<T, u8>) {
          return;
        }
        WritePRAM(address, value);
        break;
      }
      case 0x06: {
        if(std::is_same_v<T, u8>) {
          return;
        }

        switch((address >> 20) & 15) {
          case 0: case 1: WriteVRAM_PPU_BG (address, value, 0); break;
          case 2: case 3: WriteVRAM_PPU_BG (address, value, 1); break;
          case 4: case 5: WriteVRAM_PPU_OBJ(address, value, 0); break;
          case 6: case 7: WriteVRAM_PPU_OBJ(address, value, 1); break;
          default:        WriteVRAM_LCDC(address, value); break;
        }
        break;
      }
      case 0x07: {
        if(std::is_same_v<T, u8>) {
          return;
        }
        WriteOAM(address, value);
        break;
      }
      default: {
        if(address == 0x08005500 && value == 0x08005500) {
          break; // weird rockwrestler write
        }

        //ATOM_PANIC("arm9: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
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
