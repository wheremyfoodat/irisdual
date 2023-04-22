
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <dual/arm/memory.hpp>
#include <dual/nds/irq.hpp>

namespace dual::nds::arm9 {

  class DMA {
    public:
      enum class StartTime : u32 {
        Immediate = 0,
        VBlank = 1,
        HBlank = 2,
        HDraw = 3,
        MainMemoryDisplay = 4,
        Slot1 = 5,
        Slot2 = 6,
        GxFIFO = 7
      };

      DMA(arm::Memory& bus, IRQ& irq) : m_bus{bus}, m_irq{irq} {}

      void Reset();
      void Request(StartTime timing);

      u32   Read_DMASAD(int id);
      void Write_DMASAD(int id, u32 value, u32 mask);

      u32   Read_DMADAD(int id);
      void Write_DMADAD(int id, u32 value, u32 mask);

      u32   Read_DMACNT(int id);
      void Write_DMACNT(int id, u32 value, u32 mask);

      u32   Read_DMAFILL(int id);
      void Write_DMAFILL(int id, u32 value, u32 mask);

    private:
      void Run(int id);

      arm::Memory& m_bus;
      IRQ& m_irq;

      u32 m_dmasad[4]{};
      u32 m_dmadad[4]{};

      union DMACNT {
        atom::Bits< 0, 21, u32> length;
        atom::Bits<21,  2, u32> dst_address_mode;
        atom::Bits<23,  2, u32> src_address_mode;
        atom::Bits<25,  1, u32> repeat;
        atom::Bits<26,  1, u32> transfer_32bits;
        atom::Bits<27,  3, u32> timing;
        atom::Bits<30,  1, u32> enable_irq;
        atom::Bits<31,  1, u32> enable;

        u32 word = 0u;
      } m_dmacnt[4]{};

      u32 m_dmafill[4]{};

      struct Latch {
        u32 sad = 0u;
        u32 dad = 0u;
        u32 length = 0u;
      } m_latch[4]{};
  };

} // namespace dual::nds::arm9
