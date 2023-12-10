
#pragma once

#include <dual/common/scheduler.hpp>
#include <dual/nds/arm9/dma.hpp>
#include <dual/nds/video_unit/gpu/renderer/renderer_base.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>
#include <dual/nds/video_unit/gpu/registers.hpp>
#include <dual/nds/vram/vram.hpp>
#include <dual/nds/irq.hpp>
#include <memory>

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

      void Render() {
        m_renderer->Render(m_cmd_processor.GetViewport(), m_geometry_engine.GetPolygonsToRender());
      }

      void CaptureColor(int scanline, std::span<u16, 256> dst_buffer, int dst_width, bool display_capture) {
        m_renderer->CaptureColor(scanline, dst_buffer, dst_width, display_capture);
      }

      void CaptureAlpha(int scanline, std::span<int, 256> dst_buffer) {
        m_renderer->CaptureAlpha(scanline, dst_buffer);
      }

      const Matrix4<Fixed20x12>& GetClipMatrix() {
        return m_cmd_processor.GetClipMatrix();
      }

      const Matrix4<Fixed20x12>& GetVecMatrix() {
        return m_cmd_processor.GetClipMatrix();
      }

      [[nodiscard]] u32 Read_DISP3DCNT() const {
        return m_io.disp3dcnt.half;
      }

      void Write_DISP3DCNT(u16 value, u16 mask) {
        const u16 write_mask = 0x4FFFu & mask;

        m_io.disp3dcnt.half = (value & write_mask) | (m_io.disp3dcnt.half & ~write_mask);

        if(value & mask & 0x2000u) {
          m_io.disp3dcnt.poly_or_vert_ram_overflow = false;
        }
      }

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

      void Write_ALPHA_TEST_REF(u32 value, u32 mask) {
        const u32 write_mask = 0x1Fu & mask;

        m_io.alpha_test_ref = (value & write_mask) | (m_io.alpha_test_ref & ~write_mask);
      }

      void Write_CLEAR_COLOR(u32 value, u32 mask) {
        const u32 write_mask = 0x3F1FFFFFu & mask;

        m_io.clear_color.word = (value & write_mask) | (m_io.clear_color.word & ~write_mask);
      }

      void Write_CLEAR_DEPTH(u32 value, u32 mask) {
        const u32 write_mask = 0x7FFFu & mask;

        m_io.clear_depth = (value & write_mask) | (m_io.clear_depth & ~write_mask);
      }

      void SwapBuffers() {
        m_cmd_processor.SwapBuffers(m_renderer.get());
      }

      [[nodiscard]] bool GetRenderEnginePowerOn() const {
        return m_render_engine_power_on;
      }

      void SetRenderEnginePowerOn(bool power_on) {
        m_render_engine_power_on = power_on;
      }

      [[nodiscard]] bool GetGeometryEnginePowerOn() const {
        return m_geometry_engine_power_on;
      }

      void SetGeometryEnginePowerOn(bool power_on) {
        m_geometry_engine_power_on = power_on;
      }

    private:
      gpu::IO m_io;
      gpu::CommandProcessor m_cmd_processor;
      gpu::GeometryEngine m_geometry_engine;

      bool m_render_engine_power_on{};
      bool m_geometry_engine_power_on{};

      std::unique_ptr<gpu::RendererBase> m_renderer{};
  };

} // namespace dual::nds
