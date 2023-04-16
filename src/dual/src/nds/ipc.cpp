
#include <atom/logger/logger.hpp>
#include <dual/nds/ipc.hpp>

namespace dual::nds {

  IPC::IPC(IRQ& irq9, IRQ& irq7) {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;

    m_io.ipcsync.self = this;
  }

  void IPC::Reset() {
    for(auto& sync : m_sync) sync = {};
  }

  u32 IPC::IO::IPCSYNC::ReadWord(CPU cpu) {
    return self->m_sync[(int)cpu].word;
  }

  void IPC::IO::IPCSYNC::WriteWord(CPU cpu, u32 value, u32 mask) {
    auto& sync_tx = self->m_sync[(int)cpu];
    auto& sync_rx = self->m_sync[(int)cpu ^ 1];

    const u32 write_mask = 0x4F00u & mask;

    sync_tx.word = (value & write_mask) | (sync_tx.word & ~write_mask);
    sync_rx.recv = sync_tx.send;

    if(((value & mask) & 0x2000u) && sync_rx.enable_remote_irq) {
      self->m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_Sync);
    }
  }

} // namespace dual::nds
