
#pragma once

#include <dual/common/scheduler.hpp>
#include <dual/nds/arm9/dma.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>
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

      void Write_GXFIFO(u32 word) {
        m_cmd_processor.Write_GXFIFO(word);
      }

      void Write_GXCMDPORT(u32 address, u32 param) {
        m_cmd_processor.Write_GXCMDPORT(address, param);
      }

      [[nodiscard]] u32 Read_GXSTAT() const {
        return m_cmd_processor.Read_GXSTAT();
      }

      void Write_GXSTAT(u32 value, u32 mask) {
        m_cmd_processor.Write_GXSTAT(value, mask);
      }

    private:
      gpu::IO m_io;

      arm9::DMA& m_arm9_dma;
      const Region<4, 131072>& m_vram_texture;
      const Region<8>& m_vram_palette;

      gpu::CommandProcessor m_cmd_processor;
      gpu::GeometryEngine m_geometry_engine;
  };

} // namespace dual::nds
