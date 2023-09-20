
#include <dual/nds/video_unit/gpu/gpu.hpp>

namespace dual::nds {

  GPU::GPU(
    Scheduler& scheduler,
    IRQ& arm9_irq,
    arm9::DMA& arm9_dma,
    const VRAM& vram
  )   : m_scheduler{scheduler}
      , m_arm9_irq{arm9_irq}
      , m_arm9_dma{arm9_dma}
      , m_vram_texture{vram.region_gpu_texture}
      , m_vram_palette{vram.region_gpu_palette} {
  }

  void GPU::Reset() {
  }

} // namespace dual::nds
