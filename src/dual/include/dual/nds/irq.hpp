
#pragma once

#include <atom/integer.hpp>

namespace dual::nds {

  // ARM9 and ARM7 interrupt controller
  class IRQ {
    public:
      enum class Source : u32 {
        VBlank = 1 << 0,
        HBlank = 1 << 1,
        VCount = 1 << 2,
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

      void Reset() {
        // @todo
      }

      void Raise(Source source) {
        // @todo
      }
  };

} // namespace dual::nds
