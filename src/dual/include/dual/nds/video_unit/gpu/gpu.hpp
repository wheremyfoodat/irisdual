
#pragma once

#include <dual/common/scheduler.hpp>
#include <dual/nds/arm9/dma.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>
#include <dual/nds/video_unit/gpu/registers.hpp>
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

      [[nodiscard]] u32 Read_GXSTAT() const {
        return m_gxstat.word;
      }

      void Write_GXSTAT(u32 value, u32 mask) {
        const u32 write_mask = mask & 0xC0000000u;

        if(value & mask & 0x8000u) {
          // @todo: reset matrix stack pointer(s).
          m_gxstat.matrix_stack_error_flag = false;
        }

        m_gxstat.word = (m_gxstat.word & ~write_mask) | (value & write_mask);

        m_cmd_processor.UpdateIRQ();
      }

    private:

      GXSTAT m_gxstat{};

      arm9::DMA& m_arm9_dma;
      const Region<4, 131072>& m_vram_texture;
      const Region<8>& m_vram_palette;

      gpu::CommandProcessor m_cmd_processor;
  };

} // namespace dual::nds
