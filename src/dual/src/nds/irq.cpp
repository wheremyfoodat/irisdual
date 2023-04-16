
#include <dual/nds/irq.hpp>

namespace dual::nds {

  void IRQ::Reset() {
    m_reg_ime = 0u;
    m_reg_ie = 0u;
    m_reg_if = 0u;
  }

  void IRQ::SetCPU(arm::CPU* cpu) {
    m_cpu = cpu;
  }

  void IRQ::Raise(Source source) {
    m_reg_if |= (u32)source;
    UpdateIRQLine();
  }

  u32 IRQ::ReadIME() {
    return m_reg_ime;
  }

  void IRQ::WriteIME(u32 value, u32 mask) {
    const u32 write_mask = 1u & mask;

    m_reg_ime = (value & write_mask) | (m_reg_ime & ~write_mask);

    UpdateIRQLine();
  }

  u32 IRQ::ReadIE() {
    return m_reg_ie;
  }

  void IRQ::WriteIE(u32 value, u32 mask) {
    const u32 write_mask = (m_arm9 ? 0xFFFFFF7Fu : 0xFFFFFFFF) & mask;

    m_reg_ie = (value & write_mask) | (m_reg_ie & ~write_mask);

    UpdateIRQLine();
  }

  u32 IRQ::ReadIF() {
    return m_reg_if;
  }

  void IRQ::WriteIF(u32 value, u32 mask) {
    m_reg_if &= ~(value & mask);

    UpdateIRQLine();
  }

  void IRQ::UpdateIRQLine() {
    m_cpu->SetIRQFlag(m_reg_ime && (m_reg_ie & m_reg_if));
  }

} // namespace dual::nds
