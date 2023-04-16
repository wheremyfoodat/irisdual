
#include <atom/logger/logger.hpp>
#include <dual/nds/ipc.hpp>

namespace dual::nds {

  IPC::IPC(IRQ& irq9, IRQ& irq7) {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;
  }

  void IPC::Reset() {
    for(auto& ipcsync : m_ipcsync) ipcsync = {};
    for(auto& ipcfifocnt : m_ipcfifocnt) ipcfifocnt = {};
    for(auto& fifo : m_fifo) fifo.Reset();
    for(auto& fifo_latch : m_fifo_latch) fifo_latch = 0u;
  }

  u32 IPC::Read_SYNC(CPU cpu) {
    return m_ipcsync[(int)cpu].word;
  }

  void IPC::Write_SYNC(CPU cpu, u32 value, u32 mask) {
    const u32 write_mask = 0x4F00u & mask;

    auto& sync_tx = m_ipcsync[(int)cpu];
    auto& sync_rx = m_ipcsync[(int)cpu ^ 1];

    sync_tx.word = (value & write_mask) | (sync_tx.word & ~write_mask);
    sync_rx.recv = sync_tx.send;

    if(((value & mask) & 0x2000u) && sync_rx.enable_remote_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_Sync);
    }
  }

  u32 IPC::Read_FIFOCNT(CPU cpu) {
    auto const& fifo_tx = m_fifo[(int)cpu];
    auto const& fifo_rx = m_fifo[(int)cpu ^ 1];

    u32 word = m_ipcfifocnt[(int)cpu].word;

    word |= fifo_tx.IsEmpty() ?   1u : 0u;
    word |= fifo_tx.IsFull()  ?   2u : 0u;
    word |= fifo_rx.IsEmpty() ? 256u : 0u;
    word |= fifo_rx.IsFull()  ? 512u : 0u;

    return word;
  }

  void IPC::Write_FIFOCNT(CPU cpu, u32 value, u32 mask) {
    const u32 write_mask = 0x8404u & mask;

    auto& fifo_tx = m_fifo[(int)cpu];
    auto& fifo_rx = m_fifo[(int)cpu ^ 1];

    auto& fifocnt = m_ipcfifocnt[(int)cpu];

    const bool old_enable_send_fifo_irq = fifocnt.enable_send_fifo_irq;
    const bool old_enable_recv_fifo_irq = fifocnt.enable_recv_fifo_irq;

    fifocnt.word = (value & write_mask) | (fifocnt.word & ~write_mask);

    if(value & mask & 0x4000u) {
      fifocnt.error_flag = 0u;
    }

    if(value & mask & 8u) {
      fifo_tx.Reset();
    }

    if(!old_enable_send_fifo_irq && fifocnt.enable_send_fifo_irq &&  fifo_tx.IsEmpty()) {
      m_irq[(int)cpu]->Raise(IRQ::Source::IPC_SendEmpty);
    }

    if(!old_enable_recv_fifo_irq && fifocnt.enable_recv_fifo_irq && !fifo_rx.IsEmpty()) {
      m_irq[(int)cpu]->Raise(IRQ::Source::IPC_ReceiveNotEmpty);
    }
  }

  u32 IPC::Read_FIFORECV(CPU cpu) {
    auto& fifocnt = m_ipcfifocnt[(int)cpu];
    auto& fifo_rx = m_fifo[(int)cpu ^ 1];

    if(!fifocnt.enable) {
      ATOM_ERROR("{}: IPC: attempted to read FIFO but FIFOs are disabled", GetCPUName(cpu));
      return fifo_rx.Peek();
    }

    if(fifo_rx.IsEmpty()) {
      ATOM_ERROR("{}: IPC: attempted to read an empty FIFO", GetCPUName(cpu));
      fifocnt.error_flag = 1u;
      return m_fifo_latch[(int)cpu];
    }

    m_fifo_latch[(int)cpu] = fifo_rx.Read();

    if(fifo_rx.IsEmpty() && m_ipcfifocnt[(int)cpu ^ 1].enable_send_fifo_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_SendEmpty);
    }

    return m_fifo_latch[(int)cpu];
  }

  void IPC::Write_FIFOSEND(CPU cpu, u32 value) {
    auto& fifocnt = m_ipcfifocnt[(int)cpu];
    auto& fifo_tx = m_fifo[(int)cpu];

    if(!fifocnt.enable) {
      ATOM_ERROR("{}: IPC: attempted to write FIFO but FIFOs are disabled", GetCPUName(cpu));
      return;
    }

    if(fifo_tx.IsFull()) {
      fifocnt.error_flag = 1u;
      ATOM_ERROR("{}: IPC: attempted to write to an already full FIFO", GetCPUName(cpu));
      return;
    }

    if(fifo_tx.IsEmpty() && m_ipcfifocnt[(int)cpu ^ 1].enable_recv_fifo_irq) {
      m_irq[(int)cpu ^ 1]->Raise(IRQ::Source::IPC_ReceiveNotEmpty);
    }

    fifo_tx.Write(value);

    ATOM_INFO("{}: IPC: send 0x{:08X}", GetCPUName(cpu), value);
  }

} // namespace dual::nds
