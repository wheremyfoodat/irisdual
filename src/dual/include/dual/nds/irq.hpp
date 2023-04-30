
#pragma once

#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <dual/arm/cpu.hpp>

namespace dual::nds {

  // ARM9 and ARM7 interrupt controller
  class IRQ {
    public:
      enum class Source : u32 {
        VBlank = 1 << 0,
        HBlank = 1 << 1,
        VMatch = 1 << 2,
        Timer0 = 1 << 3,
        Timer1 = 1 << 4,
        Timer2 = 1 << 5,
        Timer3 = 1 << 6,
        DMA0 = 1 << 8,
        DMA1 = 1 << 9,
        DMA2 = 1 << 10,
        DMA3 = 1 << 11,
        KeyPad = 1 << 12,
        IPC_Sync = 1 << 16,
        IPC_SendEmpty = 1 << 17,
        IPC_ReceiveNotEmpty = 1 << 18,
        Cart_DataReady = 1 << 19,
        GXFIFO = 1 << 21,
        SPI = 1 << 23
      };

      explicit IRQ(bool arm9) : m_arm9{arm9} {}

      void Reset();
      auto GetCPU() -> arm::CPU*;
      void SetCPU(arm::CPU* cpu);
      void Raise(Source source);

      u32   Read_IME();
      void Write_IME(u32 value, u32 mask);

      u32   Read_IE();
      void Write_IE(u32 value, u32 mask);

      u32   Read_IF();
      void Write_IF(u32 value, u32 mask);

    private:

      void UpdateIRQLine();

      bool m_arm9;
      arm::CPU* m_cpu{};

      u32 m_reg_ime{};
      u32 m_reg_ie{};
      u32 m_reg_if{};
  };

} // namespace dual::nds
