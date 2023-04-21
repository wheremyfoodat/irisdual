
#include <dual/nds/video_unit/video_unit.hpp>

namespace dual::nds {

  VideoUnit::VideoUnit(Scheduler& scheduler, IRQ& irq9, IRQ& irq7) : m_scheduler{scheduler} {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;
  }

  void VideoUnit::Reset() {
    for(auto& dispstat : m_dispstat) dispstat = {};
  }

  void VideoUnit::UpdateVerticalCounterMatchFlag(CPU cpu) {
    auto& dispstat = m_dispstat[(int)cpu];

    const bool new_vmatch_flag = m_vcount == dispstat.vmatch_setting;

    if(dispstat.enable_vmatch_irq && !dispstat.vmatch_flag && new_vmatch_flag) {
      m_irq[(int)cpu]->Raise(IRQ::Source::VMatch);
    }

    dispstat.vmatch_flag = new_vmatch_flag;
  }

  u16 VideoUnit::Read_DISPSTAT(CPU cpu) {
    return m_dispstat[(int)cpu].half;
  }

  void VideoUnit::Write_DISPSTAT(CPU cpu, u16 value, u16 mask) {
    const u32 write_mask = 0xFFB8u & mask;

    auto& dispstat = m_dispstat[(int)cpu];

    dispstat.half = (value & write_mask) | (dispstat.half & ~write_mask);

    UpdateVerticalCounterMatchFlag(cpu);
  }

  u16 VideoUnit::Read_VCOUNT() {
    return m_vcount;
  }

} // namespace dual::nds
