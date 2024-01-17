
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
      , m_vram{memory.vram}
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
    m_dispcapcnt = {};
    m_display_swap_latch = false;
    m_display_capture_active = false;

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

      for(auto& ppu : m_ppu) ppu.SwapBuffers();

      m_display_swap_latch = m_powcnt1.enable_display_swap;
      m_display_capture_active = m_dispcapcnt.capture_enable;
      m_vcount = 0u;
    }

    UpdateVerticalCounterMatchFlag(CPU::ARM9);
    UpdateVerticalCounterMatchFlag(CPU::ARM7);

    if(m_vcount < k_drawing_lines) {
      // @todo: check if display capture reads BG+3D
      m_ppu[0].OnDrawScanlineBegin(m_vcount, m_display_capture_active);
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

        m_dispcapcnt.capture_enable = false;
        m_display_capture_active = false;
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

      if(m_display_capture_active) {
        RunDisplayCapture();
      }
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

  u32 VideoUnit::Read_DISPCAPCNT() {
    return m_dispcapcnt.word;
  }

  void VideoUnit::Write_DISPCAPCNT(u32 value, u32 mask) {
    const u32 write_mask = 0xEF3F1F1Fu & mask;

    m_dispcapcnt.word = (value & write_mask) | (m_dispcapcnt.word & ~write_mask);
  }

  void VideoUnit::RunDisplayCapture() {
    constexpr int k_capture_size_lut[4][2] {
      {128, 128}, {256,  64}, {256, 128}, {256, 192}
    };

    const uint size = m_dispcapcnt.capture_size;

    const int height = k_capture_size_lut[size][1];

    if(m_vcount >= height) {
      return;
    }

    const int width = k_capture_size_lut[size][0];

    const u32 line_offset = m_vcount * 256;

    const u32 vram_write_base    = m_dispcapcnt.vram_write_block  << 17;
    const u32 vram_write_offset  = m_dispcapcnt.vram_write_offset << 15;
    const u32 vram_write_address = vram_write_base + ((vram_write_offset + line_offset * sizeof(u16)) & 0x1FFFFu);

    u16* const buffer_dst = m_vram.region_lcdc.GetUnsafePointer<u16>(vram_write_address);

    if(buffer_dst == nullptr) [[unlikely]] {
      return;
    }

    m_ppu[0].WaitForRenderWorker();

    const auto capture_a = [&](u16* dst) {
      if(m_dispcapcnt.src_a == DISPCAPCNT::SourceA::GPUAndPPU) {
        std::memcpy(dst, m_ppu[0].GetLayerMergeOutput(), sizeof(u16) * width);
      } else {
        // @todo: perhaps a fixed-size span is not ideal for this.
        m_gpu.CaptureColor(m_vcount, std::span<u16, 256>{dst, 256}, width, true);
      }
    };

    const auto capture_b = [&](u16* dst) {
      if(m_dispcapcnt.src_b == DISPCAPCNT::SourceB::VRAM) {
        const u32 vram_read_base    = m_ppu[0].m_mmio.dispcnt.vram_block << 17;
        const u32 vram_read_offset  = m_dispcapcnt.vram_read_offset << 15;
        const u32 vram_read_address = vram_read_base + ((vram_read_offset + line_offset * sizeof(u16)) & 0x1FFFFu);

        const u16* const src = m_vram.region_lcdc.GetUnsafePointer<u16>(vram_read_address);

        if(src != nullptr) [[likely]] {
          std::memcpy(dst, src, sizeof(u16) * width);
        } else {
          std::memset(dst, 0, sizeof(u16) * width);
        }
      } else {
        ATOM_PANIC("unhandled main memory display FIFO capture");
      }
    };

    switch((DISPCAPCNT::CaptureSource)m_dispcapcnt.capture_src) {
      case DISPCAPCNT::CaptureSource::A: {
        capture_a(buffer_dst);
        break;
      }
      case DISPCAPCNT::CaptureSource::B: {
        capture_b(buffer_dst);
        break;
      }
      default: {
        u16 buffer_a[width];
        u16 buffer_b[width];

        capture_a(buffer_a);
        capture_b(buffer_b);

        const int eva = std::min((int)m_dispcapcnt.eva, 16);
        const int evb = std::min((int)m_dispcapcnt.evb, 16);
        const bool need_clamp = (eva + evb) > 16;

        for(int x = 0; x < width; x++) {
          const u16 color_a = buffer_a[x];
          const u16 color_b = buffer_b[x];

          const int r_a = (color_a >>  0) & 31;
          const int g_a = (color_a >>  5) & 31;
          const int b_a = (color_a >> 10) & 31;
          const int a_a =  color_a >> 15;

          const int r_b = (color_b >>  0) & 31;
          const int g_b = (color_b >>  5) & 31;
          const int b_b = (color_b >> 10) & 31;
          const int a_b =  color_b >> 15;

          const int factor_a = a_a * eva;
          const int factor_b = a_b * evb;

          int r_out = (r_a * factor_a + r_b * factor_b + 8) >> 4;
          int g_out = (g_a * factor_a + g_b * factor_b + 8) >> 4;
          int b_out = (b_a * factor_a + b_b * factor_b + 8) >> 4;

          if(need_clamp) {
            r_out = std::min(r_out, 15);
            g_out = std::min(g_out, 15);
            b_out = std::min(b_out, 15);
          }

          const int a_out = (eva > 0 ? a_a : 0) | (evb > 0 ? a_b : 0);

          buffer_dst[x] = r_out | (g_out << 5) | (b_out << 10) | (a_out << 15);
        }
        break;
      }
    }

    m_ppu[0].OnWriteVRAM_LCDC(vram_write_address, vram_write_address + width * sizeof(u16));
  }

} // namespace dual::nds
