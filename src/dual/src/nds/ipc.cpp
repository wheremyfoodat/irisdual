
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
    auto& sync_tx = self->m_sync[(int)cpu];
    auto& sync_rx = self->m_sync[(int)cpu ^ 1];

    return sync_rx.send | (sync_tx.enable_irq_from_remote ? 0x4000u : 0u);
  }

  void IPC::IO::IPCSYNC::WriteWord(CPU cpu, u32 value, u32 mask) {
    auto& sync_tx = self->m_sync[(int)cpu];
    auto& sync_rx = self->m_sync[(int)cpu ^ 1];

    if(mask & 0xFF00u) {
      sync_tx.send = (value >> 8) & 0xFu;
      sync_tx.enable_irq_from_remote = value & (1 << 14);

      if(sync_rx.enable_irq_from_remote) {
        self->m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_Sync);
      }
    }
  }

} // namespace dual::nds
