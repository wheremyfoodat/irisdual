
#pragma once

#include <array>
#include <dual/nds/video_unit/gpu/renderer/renderer_base.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>
#include <dual/nds/video_unit/gpu/registers.hpp>

namespace dual::nds::gpu {

  class SoftwareRenderer final : public RendererBase {
    public:
      explicit SoftwareRenderer(IO& io);

      void Render(const Viewport& viewport, std::span<const Polygon> polygons) override;

      void CaptureColor(int scanline, std::span<u16, 256> dst_buffer, int dst_width, bool display_capture) override;
      void CaptureAlpha(int scanline, std::span<int, 256> dst_buffer) override;

    private:
      void RenderRearPlane();
      void RenderPolygons(const Viewport& viewport, std::span<const Polygon> polygons);
      void RenderPolygon(const Viewport& viewport, const Polygon& polygon);

      IO& m_io;
      Color4 m_frame_buffer[192][256];
  };

} // namespace dual::nds::gpu
