
#pragma once

#include <atom/bit.hpp>
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

      u32   Read_SYNC(CPU cpu);
      void Write_SYNC(CPU cpu, u32 value, u32 mask);

    private:

      union {
        atom::Bits< 0, 4, u32> recv;
        atom::Bits< 8, 4, u32> send;
        atom::Bits<14, 1, u32> enable_remote_irq;

        u32 word = 0u;
      } m_ipcsync[2];

      IRQ* m_irq[2]{};
  };

} // namespace dual::nds
