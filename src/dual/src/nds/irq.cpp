
#include <dual/nds/irq.hpp>

namespace dual::nds {

  void IRQ::Reset() {
    m_reg_ime = 0u;
    m_reg_ie = 0u;
    m_reg_if = 0u;
  }

  auto IRQ::GetCPU() -> arm::CPU* {
    return m_cpu;
  }

  void IRQ::SetCPU(arm::CPU* cpu) {
    m_cpu = cpu;
  }

  void IRQ::Raise(Source source) {
    m_reg_if |= (u32)source;
    UpdateIRQLine();
  }

  u32 IRQ::Read_IME() {
    return m_reg_ime;
  }

  void IRQ::Write_IME(u32 value, u32 mask) {
    const u32 write_mask = 1u & mask;

    m_reg_ime = (value & write_mask) | (m_reg_ime & ~write_mask);

    UpdateIRQLine();
  }

  u32 IRQ::Read_IE() {
    return m_reg_ie;
  }

  void IRQ::Write_IE(u32 value, u32 mask) {
    const u32 write_mask = (m_arm9 ? 0xFFFFFF7Fu : 0xFFFFFFFFu) & mask;

    m_reg_ie = (value & write_mask) | (m_reg_ie & ~write_mask);

    UpdateIRQLine();
  }

  u32 IRQ::Read_IF() {
    return m_reg_if;
  }

  void IRQ::Write_IF(u32 value, u32 mask) {
    m_reg_if &= ~(value & mask);

    UpdateIRQLine();
  }

  void IRQ::UpdateIRQLine() {
    const u32 ie_and_if = m_reg_ie & m_reg_if;

    if(!m_arm9 && ie_and_if) {
      m_cpu->SetWaitingForIRQ(false);
    }

    m_cpu->SetIRQFlag(m_reg_ime && ie_and_if);
  }

} // namespace dual::nds
