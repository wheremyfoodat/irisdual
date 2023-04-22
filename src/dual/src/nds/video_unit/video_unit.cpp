
#include <atom/logger/logger.hpp>
#include <dual/nds/video_unit/video_unit.hpp>

namespace dual::nds {

  VideoUnit::VideoUnit(
    Scheduler& scheduler,
    SystemMemory& memory,
    IRQ& irq9,
    IRQ& irq7,
    arm9::DMA& dma9
  )   : m_scheduler{scheduler}, m_ppu{{0, memory}, {1, memory}}, m_dma9{dma9} {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;
  }

  void VideoUnit::Reset() {
    for(auto& dispstat : m_dispstat) dispstat = {};

    m_vcount = 0xFFFFu;

    for(auto& ppu : m_ppu) ppu.Reset();

    BeginHDraw(0);
  }

  void VideoUnit::UpdateVerticalCounterMatchFlag(CPU cpu) {
    auto& dispstat = m_dispstat[(int)cpu];

    const bool new_vmatch_flag = m_vcount == (dispstat.vmatch_setting_msb << 8 | dispstat.vmatch_setting);

    if(dispstat.enable_vmatch_irq && !dispstat.vmatch_flag && new_vmatch_flag) {
      m_irq[(int)cpu]->Raise(IRQ::Source::VMatch);
    }

    dispstat.vmatch_flag = new_vmatch_flag;
  }

  void VideoUnit::BeginHDraw(int late) {
    if(++m_vcount == k_total_lines) {
      for(auto& ppu : m_ppu) ppu.WaitForRenderWorker();

      for(auto& ppu : m_ppu) ppu.SwapBuffers();

      m_vcount = 0u;
    }

    UpdateVerticalCounterMatchFlag(CPU::ARM9);
    UpdateVerticalCounterMatchFlag(CPU::ARM7);

    if(m_vcount < k_drawing_lines) {
      m_ppu[0].OnDrawScanlineBegin(m_vcount, false);
      m_ppu[1].OnDrawScanlineBegin(m_vcount, false);

      // @todo: check if this type of DMA has weird start/stop behaviour like on the GBA.
      m_dma9.Request(arm9::DMA::StartTime::HDraw);
    } else {
      m_ppu[0].OnBlankScanlineBegin(m_vcount);
      m_ppu[1].OnBlankScanlineBegin(m_vcount);

      if(m_vcount == k_drawing_lines) {
        for(auto cpu : {CPU::ARM9, CPU::ARM7}) {
          auto& dispstat = m_dispstat[(int)cpu];

          if(dispstat.enable_vblank_irq) {
            m_irq[(int)cpu]->Raise(IRQ::Source::VBlank);
          }

          m_dma9.Request(arm9::DMA::StartTime::VBlank);

          dispstat.vblank_flag = true;
        }
      }

      if(m_vcount == k_total_lines - 1) {
        m_dispstat[(int)CPU::ARM9].vblank_flag = false;
        m_dispstat[(int)CPU::ARM7].vblank_flag = false;
      }
    }

    m_dispstat[(int)CPU::ARM9].hblank_flag = false;
    m_dispstat[(int)CPU::ARM7].hblank_flag = false;

    m_scheduler.Add(1606 - late, this, &VideoUnit::BeginHBlank);
  }

  void VideoUnit::BeginHBlank(int late) {
    for(auto cpu : {CPU::ARM9, CPU::ARM7}) {
      auto& dispstat = m_dispstat[(int)cpu];

      if(dispstat.enable_hblank_irq) {
        m_irq[(int)cpu]->Raise(IRQ::Source::HBlank);
      }

      dispstat.hblank_flag = true;
    }

    if(m_vcount < k_drawing_lines) {
      for(auto& ppu : m_ppu) ppu.OnDrawScanlineEnd();

      m_dma9.Request(arm9::DMA::StartTime::HBlank);
    }

    m_scheduler.Add(524 - late, this, &VideoUnit::BeginHDraw);
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
