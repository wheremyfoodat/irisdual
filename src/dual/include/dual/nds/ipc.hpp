
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

      struct IO {
        struct IPCSYNC {
          IPC* self{};

          u32  ReadWord(CPU cpu);
          void WriteWord(CPU cpu, u32 value, u32 mask);
        } ipcsync{};
      } m_io;

    private:

      struct {
        bool enable_irq_from_remote = false;
        uint send = 0u;
      } m_sync[2];

      IRQ* m_irq[2]{};
  };

} // namespace dual::nds
