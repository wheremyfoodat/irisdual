
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

    const auto Unhandled = [&]() {
      const int access_size = GetAccessSize(mask);
      const u32 access_address = address + GetAccessAddressOffset(mask);

      ATOM_ERROR("arm9: IO: unhandled {}-bit read from 0x{:08X}", access_size, access_address);
    };

    switch(REG(address)) {
      // IPC
      case REG(0x04000180): return hw.ipc.Read_SYNC(CPU::ARM9);
      case REG(0x04000184): return hw.ipc.Read_FIFOCNT(CPU::ARM9);
      case REG(0x04100000): return hw.ipc.Read_FIFORECV(CPU::ARM9);

      // IRQ
      case REG(0x04000208): return hw.irq.Read_IME();
      case REG(0x04000210): return hw.irq.Read_IE();
      case REG(0x04000214): return hw.irq.Read_IF();

      // VRAMCNT and WRAMCNT
      case REG(0x04000240): {
        if(mask & 0x000000FFu) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::A) <<  0;
        if(mask & 0x0000FF00u) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::B) <<  8;
        if(mask & 0x00FF0000u) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::C) << 16;
        if(mask & 0xFF000000u) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::D) << 24;
        return value;
      }
      case REG(0x04000244): {
        if(mask & 0x000000FFu) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::E) <<  0;
        if(mask & 0x0000FF00u) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::F) <<  8;
        if(mask & 0x00FF0000u) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::G) << 16;
        if(mask & 0xFF000000u) value |= hw.swram.Read_WRAMCNT() << 24;
        return value;
      }
      case REG(0x04000248): {
        if(mask & 0x000000FFu) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::H) << 0;
        if(mask & 0x0000FF00u) value |= hw.vram.Read_VRAMCNT(VRAM::Bank::I) << 8;
        return value;
      }

      // ARM9 Math
      case REG(0x04000280): return hw.math.Read_DIVCNT();
      case REG(0x04000290): return hw.math.Read_DIV_NUMER() >>  0;
      case REG(0x04000294): return hw.math.Read_DIV_NUMER() >> 32;
      case REG(0x04000298): return hw.math.Read_DIV_DENOM() >>  0;
      case REG(0x0400029C): return hw.math.Read_DIV_DENOM() >> 32;
      case REG(0x040002A0): return hw.math.Read_DIV_RESULT() >>  0;
      case REG(0x040002A4): return hw.math.Read_DIV_RESULT() >> 32;
      case REG(0x040002A8): return hw.math.Read_DIVREM_RESULT() >>  0;
      case REG(0x040002AC): return hw.math.Read_DIVREM_RESULT() >> 32;
      case REG(0x040002B0): return hw.math.Read_SQRTCNT();
      case REG(0x040002B4): return hw.math.Read_SQRT_RESULT();
      case REG(0x040002B8): return hw.math.Read_SQRT_PARAM() >>  0;
      case REG(0x040002BC): return hw.math.Read_SQRT_PARAM() >> 32;

      default: {
        Unhandled();
      }
    }

    return 0;
  }

  template<u32 mask> void MemoryBus::IO::WriteWord(u32 address, u32 value) {
    const auto Unhandled = [&]() {
      const int access_size = GetAccessSize(mask);
      const u32 access_address = address + GetAccessAddressOffset(mask);
      const u32 access_value = (value & mask) >> (GetAccessAddressOffset(mask) * 8);

      ATOM_ERROR("arm9: IO: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", access_size, access_address, access_value);
    };

    switch(REG(address)) {
      // IPC
      case REG(0x04000180): hw.ipc.Write_SYNC(CPU::ARM9, value, mask); break;
      case REG(0x04000184): hw.ipc.Write_FIFOCNT(CPU::ARM9, value, mask); break;
      case REG(0x04000188): hw.ipc.Write_FIFOSEND(CPU::ARM9, value); break;

      // IRQ
      case REG(0x04000208): hw.irq.Write_IME(value, mask); break;
      case REG(0x04000210): hw.irq.Write_IE(value, mask); break;
      case REG(0x04000214): hw.irq.Write_IF(value, mask); break;

      // VRAMCNT and WRAMCNT
      case REG(0x04000240): {
        if(mask & 0x000000FFu) hw.vram.Write_VRAMCNT(VRAM::Bank::A, value >>  0);
        if(mask & 0x0000FF00u) hw.vram.Write_VRAMCNT(VRAM::Bank::B, value >>  8);
        if(mask & 0x00FF0000u) hw.vram.Write_VRAMCNT(VRAM::Bank::C, value >> 16);
        if(mask & 0xFF000000u) hw.vram.Write_VRAMCNT(VRAM::Bank::D, value >> 24);
        break;
      }
      case REG(0x04000244): {
        if(mask & 0x000000FFu) hw.vram.Write_VRAMCNT(VRAM::Bank::E, value >>  0);
        if(mask & 0x0000FF00u) hw.vram.Write_VRAMCNT(VRAM::Bank::F, value >>  8);
        if(mask & 0x00FF0000u) hw.vram.Write_VRAMCNT(VRAM::Bank::G, value >> 16);
        if(mask & 0xFF000000u) hw.swram.Write_WRAMCNT(value >> 24);
        break;
      }
      case REG(0x04000248): {
        if(mask & 0x000000FFu) hw.vram.Write_VRAMCNT(VRAM::Bank::H, value >> 0);
        if(mask & 0x0000FF00u) hw.vram.Write_VRAMCNT(VRAM::Bank::I, value >> 8);
        break;
      }

      // ARM9 Math
      case REG(0x04000280): hw.math.Write_DIVCNT(value, mask); break;
      case REG(0x04000290): hw.math.Write_DIV_NUMER((u64)value <<  0, ((u64)mask <<  0) & 0x00000000FFFFFFFFu); break;
      case REG(0x04000294): hw.math.Write_DIV_NUMER((u64)value << 32, ((u64)mask << 32) & 0xFFFFFFFF00000000u); break;
      case REG(0x04000298): hw.math.Write_DIV_DENOM((u64)value <<  0, ((u64)mask <<  0) & 0x00000000FFFFFFFFu); break;
      case REG(0x0400029C): hw.math.Write_DIV_DENOM((u64)value << 32, ((u64)mask << 32) & 0xFFFFFFFF00000000u); break;
      case REG(0x040002B0): hw.math.Write_SQRTCNT(value, mask); break;
      case REG(0x040002B8): hw.math.Write_SQRT_PARAM((u64)value <<  0, ((u64)mask <<  0) & 0x00000000FFFFFFFFu); break;
      case REG(0x040002BC): hw.math.Write_SQRT_PARAM((u64)value << 32, ((u64)mask << 32) & 0xFFFFFFFF00000000u); break;

      default: {
        Unhandled();
      }
    }
  }

} // namespace dual::nds::arm9
