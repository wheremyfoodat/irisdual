
#include <atom/logger/logger.hpp>
#include <dual/nds/ipc.hpp>

namespace dual::nds {

  IPC::IPC(IRQ& irq9, IRQ& irq7) {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;
  }

  void IPC::Reset() {
    for(auto& ipcsync : m_ipcsync) ipcsync = {};
  }

  u32 IPC::Read_IPCSYNC(CPU cpu) {
    return m_ipcsync[(int)cpu].word;
  }

  void IPC::Write_IPCSYNC(CPU cpu, u32 value, u32 mask) {
    const u32 write_mask = 0x4F00u & mask;

    auto& sync_tx = m_ipcsync[(int)cpu];
    auto& sync_rx = m_ipcsync[(int)cpu ^ 1];

    sync_tx.word = (value & write_mask) | (sync_tx.word & ~write_mask);
    sync_rx.recv = sync_tx.send;

    if(((value & mask) & 0x2000u) && sync_rx.enable_remote_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_Sync);
    }
  }

} // namespace dual::nds
