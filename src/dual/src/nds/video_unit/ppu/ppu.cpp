
#include <atom/panic.hpp>
#include <algorithm>
#include <cstring>
#include <dual/nds/video_unit/ppu/ppu.hpp>

namespace dual::nds {

  PPU::PPU(int id, SystemMemory& memory)
      : m_vram_bg{memory.vram.region_ppu_bg[id]}
      , m_vram_obj{memory.vram.region_ppu_obj[id]}
      , m_extpal_bg{memory.vram.region_ppu_bg_extpal[id]}
      , m_extpal_obj{memory.vram.region_ppu_obj_extpal[id]}
      , m_vram_lcdc{memory.vram.region_lcdc}
      , m_pram{&memory.pram[id * 0x400]}
      , m_oam{&memory.oam[id * 0x400]} {
    if(id == 0) {
      m_mmio.dispcnt = DisplayControl{0xFFFFFFFFu};
    } else {
      m_mmio.dispcnt = DisplayControl{0xC033FFF7u};
    }
    Reset();
    RegisterMapUnmapCallbacks();
  }

  PPU::~PPU() {
    StopRenderWorker();
  }

  void PPU::Reset() {
    std::memset(m_output, 0, sizeof(m_output));

    m_mmio.dispcnt = {};

    for(auto& bgcnt : m_mmio.bgcnt) bgcnt = {};
    for(auto& bghofs : m_mmio.bghofs) bghofs = {};
    for(auto& bgvofs : m_mmio.bgvofs) bgvofs = {};

    for(auto& bgpa : m_mmio.bgpa) bgpa.half = 0x0100;
    for(auto& bgpb : m_mmio.bgpb) bgpb.half = 0x0000;
    for(auto& bgpc : m_mmio.bgpc) bgpc.half = 0x0000;
    for(auto& bgpd : m_mmio.bgpd) bgpd.half = 0x0100;

    for(auto& bgx : m_mmio.bgx) bgx = {};
    for(auto& bgy : m_mmio.bgy) bgy = {};

    for(auto& winh : m_mmio.winh) winh = {};
    for(auto& winv : m_mmio.winv) winv = {};

    m_mmio.winin  = {};
    m_mmio.winout = {};
    m_mmio.bldcnt = {};
    m_mmio.bldalpha = {};
    m_mmio.bldy = {};
    m_mmio.mosaic = {};
    m_mmio.master_bright = {};

    m_vcount = 0;

    m_vram_bg_dirty = {0, sizeof(m_render_vram_bg)};
    m_vram_obj_dirty = {0, sizeof(m_render_vram_obj)};
    m_extpal_bg_dirty = {0, sizeof(m_render_extpal_bg)};
    m_extpal_obj_dirty = {0, sizeof(m_render_extpal_obj)};
    m_vram_lcdc_dirty = {0, sizeof(m_render_vram_lcdc)};
    m_pram_dirty = {0,sizeof(m_render_pram)};
    m_oam_dirty = {0, sizeof(m_render_oam)};

    SetupRenderWorker();
  }

  void PPU::OnDrawScanlineBegin(u16 vcount, bool capture_bg_and_3d) {
    m_vcount = vcount;

    SubmitScanline(vcount, capture_bg_and_3d);
  }

  void PPU::OnDrawScanlineEnd() {
    auto& dispcnt = m_mmio.dispcnt;
    auto& bgx = m_mmio.bgx;
    auto& bgy = m_mmio.bgy;
    auto& mosaic = m_mmio.mosaic;

    // Advance vertical background mosaic counter
    if(++mosaic.bg.counter_y == mosaic.bg.size_y) {
      mosaic.bg.counter_y = 0;
    }

    // Advance vertical OBJ mosaic counter
    if(++mosaic.obj.counter_y == mosaic.obj.size_y) {
      mosaic.obj.counter_y = 0;
    }

    /* Mode 0 doesn't have any affine backgrounds,
     * in that case the internal registers seemingly aren't updated.
     * TODO: needs more research, e.g. what happens if all affine backgrounds are disabled?
     */
    if(dispcnt.bg_mode != 0) {
      // Advance internal affine registers and apply vertical mosaic if applicable.
      for(int i = 0; i < 2; i++) {
        if(m_mmio.bgcnt[2 + i].enable_mosaic) {
          if(mosaic.bg.counter_y == 0) {
            bgx[i].current += mosaic.bg.size_y * (s16)m_mmio.bgpb[i].half;
            bgy[i].current += mosaic.bg.size_y * (s16)m_mmio.bgpd[i].half;
          }
        } else {
          bgx[i].current += (s16)m_mmio.bgpb[i].half;
          bgy[i].current += (s16)m_mmio.bgpd[i].half;
        }
      }
    }
  }

  void PPU::OnBlankScanlineBegin(u16 vcount) {
    auto& bgx = m_mmio.bgx;
    auto& bgy = m_mmio.bgy;
    auto& mosaic = m_mmio.mosaic;

    m_vcount = vcount;

    // TODO: when exactly are these registers reloaded?
    if(vcount == 192) {
      // Reset vertical mosaic counters
      mosaic.bg.counter_y = 0;
      mosaic.obj.counter_y = 0;

      // Reload internal affine registers
      bgx[0].current = (s32)bgx[0].initial;
      bgy[0].current = (s32)bgy[0].initial;
      bgx[1].current = (s32)bgx[1].initial;
      bgy[1].current = (s32)bgy[1].initial;
    }

    SubmitScanline(vcount, false);
  }

  void PPU::RenderScanline(u16 vcount, bool capture_bg_and_3d) {
    auto display_mode = m_mmio_copy[vcount].dispcnt.display_mode;

    if(capture_bg_and_3d || display_mode == 1) {
      RenderBackgroundsAndComposite(vcount);
    }

    switch(display_mode) {
      case 0: RenderDisplayOff(vcount); break;
      case 1: RenderNormal(vcount);     break;
      case 2: RenderVideoMemoryDisplay(vcount); break;
      case 3: RenderMainMemoryDisplay(vcount);  break;
    }
  }

  void PPU::RenderDisplayOff(u16 vcount) {
    u32* line = &m_output[m_frame][vcount * 256];

    for(uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(0x7FFF);
    }
  }

  void PPU::RenderNormal(u16 vcount) {
    u32* line = &m_output[m_frame][vcount * 256];

    for(uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(m_buffer_compose[x]);
    }

    RenderMasterBrightness(vcount);
  }

  void PPU::RenderVideoMemoryDisplay(u16 vcount) {
    u32* line = &m_output[m_frame][vcount * 256];
    auto vram_block = m_mmio_copy[vcount].dispcnt.vram_block;
    u16 const* source = (u16 const*)&m_render_vram_lcdc[vram_block * 0x20000 + vcount * 256 * sizeof(u16)];

    if(source != nullptr) {
      for(uint x = 0; x < 256; x++) {
        line[x] = ConvertColor(*source++);
      }
    } else {
      for(uint x = 0; x < 256; x++) {
        line[x] = ConvertColor(0);
      }
    }

    RenderMasterBrightness(vcount);
  }

  void PPU::RenderMasterBrightness(int vcount) {
    auto const& master_bright = m_mmio_copy[vcount].master_bright;

    if(master_bright.mode != MasterBrightness::Mode::Off && master_bright.factor != 0) {
      int  factor = std::min((int)master_bright.factor, 16);
      u32* buffer = &m_output[m_frame][vcount * 256];

      if(master_bright.mode == MasterBrightness::Mode::Up) {
        for(int x = 0; x < 256; x++) {
          u32 rgba = *buffer;
          u32 rgba_inv = ~rgba;

          rgba += ((((rgba_inv & 0xFF00FF) * factor) & 0xFF00FF0) |
                   (((rgba_inv & 0x00FF00) * factor) & 0x00FF000)) >> 4;
          *buffer++ = rgba;
        }
      } else {
        for(int x = 0; x < 256; x++) {
          u32 rgba = *buffer;

          rgba -= ((((rgba & 0xFF00FF) * factor) & 0xFF00FF0) |
                   (((rgba & 0x00FF00) * factor) & 0x00FF000)) >> 4;
          *buffer++ = rgba;
        }
      }
    }
  }

  void PPU::RenderMainMemoryDisplay(u16 vcount) {
    ATOM_PANIC("PPU: unimplemented main memory display mode.");

    //RenderMasterBrightness(vcount);
  }

  void PPU::RenderBackgroundsAndComposite(u16 vcount) {
    auto const& mmio = m_mmio_copy[vcount];

    if(mmio.dispcnt.forced_blank) {
      for(uint x = 0; x < 256; x++) {
        m_buffer_compose[x] = 0xFFFF;
      }
      return;
    }

    // TODO: on a real Nintendo DS all sprites are rendered one scanline ahead.
    if(mmio.dispcnt.enable[ENABLE_OBJ]) {
      RenderLayerOAM(vcount);
    }

    if(mmio.dispcnt.enable[ENABLE_BG0]) {
      // TODO: what does HW do if "enable BG0 3D" is disabled in mode 6.
      if(mmio.dispcnt.enable_bg0_3d || mmio.dispcnt.bg_mode == 6) {
        // gpu->CaptureColor(buffer_bg[0], vcount, 256, false);
        // gpu->CaptureAlpha(buffer_3d_alpha, vcount);
      } else {
        RenderLayerText(0, vcount);
      }
    }

    if(mmio.dispcnt.enable[ENABLE_BG1] && mmio.dispcnt.bg_mode != 6) {
      RenderLayerText(1, vcount);
    }

    if(mmio.dispcnt.enable[ENABLE_BG2]) {
      switch(mmio.dispcnt.bg_mode) {
        case 0:
        case 1:
        case 3: RenderLayerText(2, vcount); break;
        case 2:
        case 4: RenderLayerAffine(0, vcount); break;
        case 5: RenderLayerExtended(0, vcount); break;
        case 6: RenderLayerLarge(vcount); break;
      }
    }

    if(mmio.dispcnt.enable[ENABLE_BG3]) {
      switch(mmio.dispcnt.bg_mode) {
        case 0: RenderLayerText(3, vcount); break;
        case 1:
        case 2: RenderLayerAffine(1, vcount); break;
        case 3:
        case 4:
        case 5: RenderLayerExtended(1, vcount); break;
      }
    }

    ComposeScanline(vcount, 0, 3);
  }

  void PPU::SetupRenderWorker() {
    StopRenderWorker();

    m_render_worker.vcount = 0;
    m_render_worker.vcount_max = -1;
    m_render_worker.running = true;
    m_render_worker.ready = false;

    m_render_worker.thread = std::thread([this]() {
      while(m_render_worker.running.load()) {
        while(m_render_worker.vcount <= m_render_worker.vcount_max) {
          // TODO: this might be racy with SubmitScanline() resetting render_thread_vcount.
          int vcount = m_render_worker.vcount;

          if(m_mmio.dispcnt.enable[ENABLE_WIN0]) {
            RenderWindow(0, vcount);
          }

          if(m_mmio.dispcnt.enable[ENABLE_WIN1]) {
            RenderWindow(1, vcount);
          }

          if(vcount < 192) {
            RenderScanline(vcount, m_mmio_copy[vcount].capture_bg_and_3d);
          }

          m_render_worker.vcount++;
        }

        // Wait for the emulation thread to submit more work:
        std::unique_lock lock{m_render_worker.mutex};
        m_render_worker.cv.wait(lock, [this]() {return m_render_worker.ready;});
        m_render_worker.ready = false;
      }
    });
  }

  void PPU::StopRenderWorker() {
    if(!m_render_worker.running) {
      return;
    }

    // Wake the render worker thread up if it is working for new data:
    m_render_worker.mutex.lock();
    m_render_worker.ready = true;
    m_render_worker.cv.notify_one();
    m_render_worker.mutex.unlock();

    // Tell the render worker thread to quit and join it:
    m_render_worker.running = false;
    m_render_worker.thread.join();
  }

  void PPU::SubmitScanline(u16 vcount, bool capture_bg_and_3d) {
    m_mmio.capture_bg_and_3d = capture_bg_and_3d;

    if(vcount < 192) {
      m_mmio_copy[vcount] = m_mmio;
    } else {
      m_mmio_copy[vcount].dispcnt = m_mmio.dispcnt;
      m_mmio_copy[vcount].winh[0] = m_mmio.winh[0];
      m_mmio_copy[vcount].winh[1] = m_mmio.winh[1];
      m_mmio_copy[vcount].winv[0] = m_mmio.winv[0];
      m_mmio_copy[vcount].winv[1] = m_mmio.winv[1];
    }

    if(vcount == 0) {
      CopyVRAM(m_vram_bg, m_render_vram_bg, m_vram_bg_dirty);
      CopyVRAM(m_vram_obj, m_render_vram_obj, m_vram_obj_dirty);
      CopyVRAM(m_extpal_bg, m_render_extpal_bg, m_extpal_bg_dirty);
      CopyVRAM(m_extpal_obj, m_render_extpal_obj, m_extpal_obj_dirty);
      CopyVRAM(m_vram_lcdc, m_render_vram_lcdc, m_vram_lcdc_dirty);
      CopyVRAM(m_pram, m_render_pram, m_pram_dirty);
      CopyVRAM(m_oam, m_render_oam, m_oam_dirty);

      m_vram_bg_dirty = {};
      m_vram_obj_dirty = {};
      m_extpal_bg_dirty = {};
      m_extpal_obj_dirty = {};
      m_vram_lcdc_dirty = {};
      m_pram_dirty = {};
      m_oam_dirty = {};

      m_render_worker.vcount = 0;
    }

    m_render_worker.vcount_max = vcount;

    std::lock_guard lock{m_render_worker.mutex};
    m_render_worker.ready = true;
    m_render_worker.cv.notify_one();
  }

  void PPU::RegisterMapUnmapCallbacks() {
    m_vram_bg.AddCallback([this](u32 offset, size_t size) {
      OnWriteVRAM_BG(offset, offset + size);
    });

    m_vram_obj.AddCallback([this](u32 offset, size_t size) {
      OnWriteVRAM_OBJ(offset, offset + size);
    });

    m_extpal_bg.AddCallback([this](u32 offset, size_t size) {
      OnWriteExtPal_BG(offset, offset + size);
    });

    m_extpal_obj.AddCallback([this](u32 offset, size_t size) {
      OnWriteExtPal_OBJ(offset, offset + size);
    });

    m_vram_lcdc.AddCallback([this](u32 offset, size_t size) {
      OnWriteVRAM_LCDC(offset, offset + size);
    });
  }

} // namespace dual::nds
