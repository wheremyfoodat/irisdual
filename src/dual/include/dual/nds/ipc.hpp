
#pragma once

#include <atom/bit.hpp>
#include <dual/common/fifo.hpp>
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

      u32   Read_FIFOCNT(CPU cpu);
      void Write_FIFOCNT(CPU cpu, u32 value, u32 mask);

      u32   Read_FIFORECV(CPU cpu);
      void Write_FIFOSEND(CPU cpu, u32 value);

    private:

      static constexpr auto GetCPUName(CPU cpu) {
        return cpu == CPU::ARM9 ? "arm9" : "arm7";
      }

      union {
        atom::Bits< 0, 4, u32> recv;
        atom::Bits< 8, 4, u32> send;
        atom::Bits<14, 1, u32> enable_remote_irq;

        u32 word = 0u;
      } m_ipcsync[2];

      union {
        atom::Bits< 2, 1, u32> enable_send_fifo_irq;
        atom::Bits<10, 1, u32> enable_recv_fifo_irq;
        atom::Bits<14, 1, u32> error_flag;
        atom::Bits<15, 1, u32> enable;

        u32 word = 0u;
      } m_ipcfifocnt[2];

      FIFO<u32, 16> m_fifo[2];
      u32 m_fifo_latch[2];

      IRQ* m_irq[2]{};
  };

} // namespace dual::nds
