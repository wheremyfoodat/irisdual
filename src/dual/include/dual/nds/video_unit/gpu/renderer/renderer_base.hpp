
#pragma once

// @todo: move the Polygon and Vertex definitions outside the geometry engine header.
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>

#include <span>

namespace dual::nds::gpu {

  struct Viewport {
    int x0;
    int y0;
    int width;
    int height;
  };

  class RendererBase {
    public:
      virtual ~RendererBase() = default;

      virtual void SetWBufferEnable(bool enable_w_buffer) = 0;
      virtual void UpdateEdgeColor(size_t table_offset, std::span<const u32> table_data) = 0;
      virtual void UpdateToonTable(size_t table_offset, std::span<const u32> table_data) = 0;

      virtual void Render(const Viewport& viewport, std::span<const Polygon* const> polygons) = 0;

      virtual void CaptureColor(int scanline, std::span<u16, 256> dst_buffer, int dst_width, bool display_capture) = 0;
      virtual void CaptureAlpha(int scanline, std::span<int, 256> dst_buffer) = 0;
  };

} // namespace dual::nds::gpu
