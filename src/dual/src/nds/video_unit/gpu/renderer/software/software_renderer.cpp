
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>

namespace dual::nds::gpu {

  SoftwareRenderer::SoftwareRenderer(IO& io) : m_io{io} {
  }

  void SoftwareRenderer::Render(const Viewport& viewport, std::span<const Polygon> polygons) {
    RenderRearPlane();
    RenderPolygons(viewport, polygons);
  }

  void SoftwareRenderer::CaptureColor(int scanline, std::span<u16, 256> dst_buffer, int dst_width, bool display_capture) {
    // @todo: write a separate method for display capture?

    for(int x = 0; x < dst_width; x++) {
      const Color4& color = m_frame_buffer[scanline][x];

      if(display_capture) {
        dst_buffer[x] = color.ToRGB555() | (color.A() != 0 ? 0x8000u : 0u);
      } else {
        dst_buffer[x] = color.A() == 0 ? 0x8000u : color.ToRGB555();
      }
    }
  }

  void SoftwareRenderer::CaptureAlpha(int scanline, std::span<int, 256> dst_buffer) {
    for(int x = 0; x < 256; x++) {
      dst_buffer[x] = m_frame_buffer[scanline][x].A().Raw() >> 2;
    }
  }

} // namespace dual::nds::gpu
