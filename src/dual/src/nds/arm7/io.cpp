
#include <dual/nds/arm7/memory.hpp>

#define REG(address) ((address) >> 2)

namespace dual::nds::arm7 {

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

      ATOM_ERROR("arm7: IO: unhandled {}-bit read from 0x{:08X}", access_size, access_address);
    };

    switch(REG(address)) {
      // PPU
      case REG(0x04000004): {
        if(mask & 0x0000FFFFu) value |= hw.video_unit.Read_DISPSTAT(CPU::ARM7) << 0;
        if(mask & 0xFFFF0000u) value |= hw.video_unit.Read_VCOUNT() << 16;
        return value;
      }

      // DMA
      case REG(0x040000B0): return hw.dma.Read_DMASAD(0);
      case REG(0x040000B4): return hw.dma.Read_DMADAD(0);
      case REG(0x040000B8): return hw.dma.Read_DMACNT(0);
      case REG(0x040000BC): return hw.dma.Read_DMASAD(1);
      case REG(0x040000C0): return hw.dma.Read_DMADAD(1);
      case REG(0x040000C4): return hw.dma.Read_DMACNT(1);
      case REG(0x040000C8): return hw.dma.Read_DMASAD(2);
      case REG(0x040000CC): return hw.dma.Read_DMADAD(2);
      case REG(0x040000D0): return hw.dma.Read_DMACNT(2);
      case REG(0x040000D4): return hw.dma.Read_DMASAD(3);
      case REG(0x040000D8): return hw.dma.Read_DMADAD(3);
      case REG(0x040000DC): return hw.dma.Read_DMACNT(3);

      // IPC
      case REG(0x04000180): return hw.ipc.Read_SYNC(CPU::ARM7);
      case REG(0x04000184): return hw.ipc.Read_FIFOCNT(CPU::ARM7);
      case REG(0x04100000): return hw.ipc.Read_FIFORECV(CPU::ARM7);

      // IRQ
      case REG(0x04000208): return hw.irq.Read_IME();
      case REG(0x04000210): return hw.irq.Read_IE();
      case REG(0x04000214): return hw.irq.Read_IF();

      // VRAMSTAT and WRAMSTAT
      case REG(0x04000240): {
        if(mask & 0x000000FFu) value |= hw.vram.Read_VRAMSTAT() << 0;
        if(mask & 0x0000FF00u) value |= hw.swram.Read_WRAMCNT() << 8;
        return value;
      }

      default: {
        Unhandled();
      }
    }

    return 0;
  }

  template<u32 mask> void MemoryBus::IO::WriteWord(u32 address, u32 value) {
    switch(REG(address)) {
      // PPU
      case REG(0x04000004): hw.video_unit.Write_DISPSTAT(CPU::ARM7, value, (u16)mask);

      // DMA
      case REG(0x040000B0): hw.dma.Write_DMASAD(0, value, mask); break;
      case REG(0x040000B4): hw.dma.Write_DMADAD(0, value, mask); break;
      case REG(0x040000B8): hw.dma.Write_DMACNT(0, value, mask); break;
      case REG(0x040000BC): hw.dma.Write_DMASAD(1, value, mask); break;
      case REG(0x040000C0): hw.dma.Write_DMADAD(1, value, mask); break;
      case REG(0x040000C4): hw.dma.Write_DMACNT(1, value, mask); break;
      case REG(0x040000C8): hw.dma.Write_DMASAD(2, value, mask); break;
      case REG(0x040000CC): hw.dma.Write_DMADAD(2, value, mask); break;
      case REG(0x040000D0): hw.dma.Write_DMACNT(2, value, mask); break;
      case REG(0x040000D4): hw.dma.Write_DMASAD(3, value, mask); break;
      case REG(0x040000D8): hw.dma.Write_DMADAD(3, value, mask); break;
      case REG(0x040000DC): hw.dma.Write_DMACNT(3, value, mask); break;

      // IPC
      case REG(0x04000180): hw.ipc.Write_SYNC(CPU::ARM7, value, mask); break;
      case REG(0x04000184): hw.ipc.Write_FIFOCNT(CPU::ARM7, value, mask); break;
      case REG(0x04000188): hw.ipc.Write_FIFOSEND(CPU::ARM7, value); break;

      // IRQ
      case REG(0x04000208): hw.irq.Write_IME(value, mask); break;
      case REG(0x04000210): hw.irq.Write_IE(value, mask); break;
      case REG(0x04000214): hw.irq.Write_IF(value, mask); break;

      default: {
        const int access_size = GetAccessSize(mask);
        const u32 access_address = address + GetAccessAddressOffset(mask);
        const u32 access_value = (value & mask) >> (GetAccessAddressOffset(mask) * 8);

        ATOM_ERROR("arm7: IO: unhandled {}-bit write to 0x{:08X} = 0x{:08X}", access_size, access_address, access_value);
      }
    }
  }

} // namespace dual::nds::arm7
