
#pragma once

#include <dual/common/scheduler.hpp>
#include <dual/nds/arm9/dma.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>
#include <dual/nds/vram/vram.hpp>
#include <dual/nds/irq.hpp>

namespace dual::nds {

  // 3D graphics processing unit (GPU)
  class GPU {
    public:
      GPU(
        Scheduler& scheduler,
        IRQ& arm9_irq,
        arm9::DMA& arm9_dma,
        const VRAM& vram
      );

      void Reset();

      gpu::CommandProcessor& GetCommandProcessor() {
        return m_cmd_processor;
      }

    private:

      Scheduler& m_scheduler;
      IRQ& m_arm9_irq;
      arm9::DMA& m_arm9_dma;
      const Region<4, 131072>& m_vram_texture;
      const Region<8>& m_vram_palette;

      gpu::CommandProcessor m_cmd_processor{};
  };

} // namespace dual::nds
