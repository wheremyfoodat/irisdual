
#include <dual/nds/arm9/memory.hpp>

#define REG(address) ((address) >> 2)

namespace dual::nds::arm9 {

  static constexpr int GetAccessSize(u32 mask) {
    int size = 0;
    u32 ff = 0xFFu;

    for(int i = 0; i < 4; i++) {
      if(mask & ff) size += 8;

      ff <<= 8;
    }

    return size;
  }

  static constexpr u32 GetAccessAddressOffset(u32 mask) {
    int offset = 0;

    for(int i = 0; i < 4; i++) {
      if(mask & 0xFFu) break;

      offset++;
      mask >>= 8;
    }

    return offset;
  }

  u8 MemoryBus::IO::ReadByte(u32 address) {
    switch(address & 3u) {
      case 0u: return ReadWord<0x000000FF>(address);;
      case 1u: return ReadWord<0x0000FF00>(address & ~3u) >>  8;
      case 2u: return ReadWord<0x00FF0000>(address & ~3u) >> 16;
      case 3u: return ReadWord<0xFF000000>(address & ~3u) >> 24;
    }
    return 0u;
  }

  u16 MemoryBus::IO::ReadHalf(u32 address) {
    switch(address & 2u) {
      case 0u: return ReadWord<0x0000FFFF>(address);
      case 2u: return ReadWord<0xFFFF0000>(address & ~2u) >> 16;
    }
    return 0u;
  }

  u32 MemoryBus::IO::ReadWord(u32 address) {
    return ReadWord<0xFFFFFFFF>(address);
  }

  void MemoryBus::IO::WriteByte(u32 address, u8  value) {
    const u32 word = value * 0x01010101u;

    switch(address & 3u) {
      case 0u: WriteWord<0x000000FF>(address      , word); break;
      case 1u: WriteWord<0x0000FF00>(address & ~3u, word); break;
      case 2u: WriteWord<0x00FF0000>(address & ~3u, word); break;
      case 3u: WriteWord<0xFF000000>(address & ~3u, word); break;
    }
  }

  void MemoryBus::IO::WriteHalf(u32 address, u16 value) {
    const u32 word = value * 0x00010001u;

    switch(address & 2u) {
      case 0u: WriteWord<0x0000FFFF>(address      , word); break;
      case 2u: WriteWord<0xFFFF0000>(address & ~2u, word); break;
    }
  }

  void MemoryBus::IO::WriteWord(u32 address, u32 value) {
    WriteWord<0xFFFFFFFF>(address, value);
  }

  template<u32 mask> u32 MemoryBus::IO::ReadWord(u32 address) {
    u32 value = 0u;

    switch(REG(address)) {
      default: {
        const int access_size = GetAccessSize(mask);
        const u32 access_address = address + GetAccessAddressOffset(mask);

        ATOM_ERROR("arm9: IO: unhandled {}-bit read from 0x{:08X}", access_size, access_address);
      }
    }

    return 0;
  }

  template<u32 mask> void MemoryBus::IO::WriteWord(u32 address, u32 value) {
    switch(REG(address)) {
      case REG(0x04000000): {
        // DISPCNT
        //ppu.dispcnt.WriteWord(value, mask);
        break;
      }
      case REG(0x04000004): {
        // DISPSTAT, VCOUNT
        //if(mask & 0x0000FFFFu) ppu.dispstat.WriteWord(value >>  0, mask);
        //if(mask & 0xFFFF0000u) ppu.vcount.WriteWord(value >> 16, mask);
        break;
      }
      default: {
        const int access_size = GetAccessSize(mask);
        const int access_offset = GetAccessAddressOffset(mask);
        const u32 access_address = address + access_offset;
        const u32 access_value = (value & mask) >> (access_offset * 8);

        ATOM_ERROR("arm9: IO: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", access_size, access_address, access_value);
      }
    }
  }

} // namespace dual::nds::arm9
