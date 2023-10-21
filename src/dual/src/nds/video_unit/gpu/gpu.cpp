
#include <dual/nds/video_unit/gpu/gpu.hpp>

namespace dual::nds {

  GPU::GPU(
    Scheduler& scheduler,
    IRQ& arm9_irq,
    arm9::DMA& arm9_dma,
    const VRAM& vram
  )   : m_arm9_dma{arm9_dma}
      , m_vram_texture{vram.region_gpu_texture}
      , m_vram_palette{vram.region_gpu_palette}
      , m_cmd_processor{scheduler, arm9_irq, m_io, m_geometry_engine}
      , m_geometry_engine{m_io} {
  }

  void GPU::Reset() {
    m_cmd_processor.Reset();
    m_geometry_engine.Reset();

    m_render_engine_power_on = false;
    m_geometry_engine_power_on = false;
  }

} // namespace dual::nds
