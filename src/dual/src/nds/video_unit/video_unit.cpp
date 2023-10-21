
#include <algorithm>
#include <atom/logger/logger.hpp>
#include <dual/nds/video_unit/video_unit.hpp>

namespace dual::nds {

  VideoUnit::VideoUnit(
    Scheduler& scheduler,
    SystemMemory& memory,
    IRQ& irq9,
    IRQ& irq7,
    arm9::DMA& dma9,
    arm7::DMA& dma7
  )   : m_scheduler{scheduler}
      , m_gpu{scheduler, irq9, dma9, memory.vram}
      , m_ppu{{0, memory, &m_gpu}, {1, memory}}
      , m_dma9{dma9}
      , m_dma7{dma7} {
    m_irq[(int)CPU::ARM9] = &irq9;
    m_irq[(int)CPU::ARM7] = &irq7;
  }

  void VideoUnit::Reset() {
    for(auto& dispstat : m_dispstat) dispstat = {};

    m_vcount = 0xFFFFu;
    m_powcnt1 = {};
    m_display_swap_latch = false;

    m_gpu.Reset();
    for(auto& ppu : m_ppu) ppu.Reset();

    BeginHDraw(0);
  }

  void VideoUnit::DirectBoot() {
    /**
     * Enable the LCDs and PPU A and B. This is a mere guess based on how the DS firmware
     * might initialize this register given that it displays 2D content on both screens and
     * no 3D content.
     */
    Write_POWCNT(0x0203u, 0xFFFFu);
  }

  void VideoUnit::UpdateVerticalCounterMatchFlag(CPU cpu) {
    auto& dispstat = m_dispstat[(int)cpu];

    const bool new_vmatch_flag = m_vcount == (dispstat.vmatch_setting_msb << 8 | dispstat.vmatch_setting);

    if(dispstat.enable_vmatch_irq && !dispstat.vmatch_flag && new_vmatch_flag) {
      m_irq[(int)cpu]->Request(IRQ::Source::VMatch);
    }

    dispstat.vmatch_flag = new_vmatch_flag;
  }

  void VideoUnit::BeginHDraw(int late) {
    if(++m_vcount == k_total_lines) {
      for(auto& ppu : m_ppu) ppu.WaitForRenderWorker();

      for(auto& ppu : m_ppu) ppu.SwapBuffers();

      if(m_present_callback) [[likely]] {
        const u32* frames[2] {
          m_ppu[1].GetFrameBuffer(),
          m_ppu[0].GetFrameBuffer()
        };

        if(m_display_swap_latch) {
          std::swap(frames[0], frames[1]);
        }

        m_present_callback(frames[0], frames[1]);
      }

      m_display_swap_latch = m_powcnt1.enable_display_swap;
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
            m_irq[(int)cpu]->Request(IRQ::Source::VBlank);
          }

          m_dma9.Request(arm9::DMA::StartTime::VBlank);
          m_dma7.Request(arm7::DMA::StartTime::VBlank);

          dispstat.vblank_flag = true;
        }

        m_gpu.SwapBuffers();
      }

      // The GPU begins rendering the frame in the last 48 scanlines of V-blank.
      if(m_vcount == k_total_lines - 48) {
        /**
         * PPU A may read from the GPU's framebuffer, so make sure that it finishes its frame
         * before we start rendering the next GPU frame.
         *
         * @todo: see if using a double buffer would be more efficient instead of always syncing?
         */
        m_ppu[0].WaitForRenderWorker();

        m_gpu.Render();
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
        m_irq[(int)cpu]->Request(IRQ::Source::HBlank);
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

  u16 VideoUnit::Read_POWCNT1() {
    return m_powcnt1.half;
  }

  void VideoUnit::Write_POWCNT(u16 value, u16 mask) {
    const u16 write_mask = 0x820Fu & mask;

    m_powcnt1.half = (value & write_mask) | (m_powcnt1.half & ~write_mask);

    m_ppu[0].SetPowerOn(m_powcnt1.enable_ppu_a);
    m_ppu[1].SetPowerOn(m_powcnt1.enable_ppu_b);
    m_gpu.SetRenderEnginePowerOn(m_powcnt1.enable_gpu_render_engine);
    m_gpu.SetGeometryEnginePowerOn(m_powcnt1.enable_gpu_geometry_engine);
  }


} // namespace dual::nds
