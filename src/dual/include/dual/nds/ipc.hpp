
#pragma once

#include <dual/nds/irq.hpp>

namespace dual::nds {

  // Inter-Process Communication hardware for ARM9 and ARM7 synchronization and message passing.
  class IPC {
    public:
      enum CPU {
        ARM7 = 0,
        ARM9 = 1
      };

      IPC(IRQ& irq9, IRQ& irq7);

      void Reset();

      u32  IPCSYNC_ReadWord(CPU cpu);
      void IPCSYNC_WriteWord(CPU cpu, u32 value, u32 mask);

    private:

      IRQ* irq[2]{};
  };

} // namespace dual::nds
