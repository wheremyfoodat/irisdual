
#include <atom/logger/logger.hpp>
#include <dual/nds/ipc.hpp>

namespace dual::nds {

  IPC::IPC(IRQ& irq9, IRQ& irq7) {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;
  }

  void IPC::Reset() {
    for(auto& sync : m_sync) sync = {};
    for(auto& fifo : m_fifo) fifo = {};
  }

  u32 IPC::Read_SYNC(CPU cpu) {
    return m_sync[(int)cpu].word;
  }

  void IPC::Write_SYNC(CPU cpu, u32 value, u32 mask) {
    const u32 write_mask = 0x4F00u & mask;

    auto& sync_tx = m_sync[(int)cpu];
    auto& sync_rx = m_sync[(int)cpu ^ 1];

    sync_tx.word = (value & write_mask) | (sync_tx.word & ~write_mask);
    sync_rx.recv = sync_tx.send;

    if(((value & mask) & 0x2000u) && sync_rx.enable_remote_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_Sync);
    }
  }

  u32 IPC::Read_FIFOCNT(CPU cpu) {
    auto const& fifo_tx = m_fifo[(int)cpu];
    auto const& fifo_rx = m_fifo[(int)cpu ^ 1];

    u32 word = fifo_tx.control.word;

    word |= fifo_tx.send.IsEmpty() ?   1u : 0u;
    word |= fifo_tx.send.IsFull()  ?   2u : 0u;
    word |= fifo_rx.send.IsEmpty() ? 256u : 0u;
    word |= fifo_rx.send.IsFull()  ? 512u : 0u;

    return word;
  }

  void IPC::Write_FIFOCNT(CPU cpu, u32 value, u32 mask) {
    const u32 write_mask = 0x8404u & mask;

    auto& fifo_tx = m_fifo[(int)cpu];
    auto& fifo_rx = m_fifo[(int)cpu ^ 1];

    auto& control = fifo_tx.control;

    const bool old_enable_send_fifo_irq = control.enable_send_fifo_irq;
    const bool old_enable_recv_fifo_irq = control.enable_recv_fifo_irq;

    control.word = (value & write_mask) | (control.word & ~write_mask);

    if(value & mask & 0x4000u) {
      control.error_flag = 0u;
    }

    if(value & mask & 8u) {
      fifo_tx.send.Reset();
    }

    if(!old_enable_send_fifo_irq && control.enable_send_fifo_irq &&  fifo_tx.send.IsEmpty()) {
      m_irq[(int)cpu]->Raise(IRQ::Source::IPC_SendEmpty);
    }

    if(!old_enable_recv_fifo_irq && control.enable_recv_fifo_irq && !fifo_rx.send.IsEmpty()) {
      m_irq[(int)cpu]->Raise(IRQ::Source::IPC_ReceiveNotEmpty);
    }
  }

  u32 IPC::Read_FIFORECV(CPU cpu) {
    auto& fifo_tx = m_fifo[(int)cpu];
    auto& fifo_rx = m_fifo[(int)cpu ^ 1];

    if(!fifo_tx.control.enable) {
      ATOM_ERROR("{}: IPC: attempted to read FIFO but FIFOs are disabled", cpu);
      return fifo_rx.send.Peek();
    }

    if(fifo_rx.send.IsEmpty()) {
      ATOM_ERROR("{}: IPC: attempted to read an empty FIFO", cpu);
      fifo_tx.control.error_flag = 1u;
      return fifo_tx.latch;
    }

    fifo_tx.latch = fifo_rx.send.Read();

    if(fifo_rx.send.IsEmpty() && fifo_rx.control.enable_send_fifo_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_SendEmpty);
    }

    return fifo_tx.latch;
  }

  void IPC::Write_FIFOSEND(CPU cpu, u32 value) {
    auto& fifo_tx = m_fifo[(int)cpu];
    auto& fifo_rx = m_fifo[(int)cpu ^ 1];

    if(!fifo_tx.control.enable) {
      ATOM_ERROR("{}: IPC: attempted to write FIFO but FIFOs are disabled", cpu);
      return;
    }

    if(fifo_tx.send.IsFull()) {
      fifo_tx.control.error_flag = 1u;
      ATOM_ERROR("{}: IPC: attempted to write to an already full FIFO", cpu);
      return;
    }

    if(fifo_tx.send.IsEmpty() && fifo_rx.control.enable_recv_fifo_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_ReceiveNotEmpty);
    }

    fifo_tx.send.Write(value);
  }

} // namespace dual::nds
