
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>

namespace dual::nds::gpu {

  void SoftwareRenderer::RenderRearPlane() {
    for(int y = 0; y < 192; y++) {
      for(int x = 0; x < 256; x++) {
        m_frame_buffer[y][x] = Color4{0, 0, 63};
      }
    }
  }

  void SoftwareRenderer::RenderPolygons(const Viewport& viewport, std::span<const Polygon> polygons) {
    const int viewport_x0 = viewport.x0;
    const int viewport_y0 = viewport.y0;
    const int viewport_width = 1 + viewport.x1 - viewport_x0;
    const int viewport_height = 1 + viewport.y1 - viewport_y0;

    if(viewport.x0 > viewport.x1 || viewport.y0 > viewport.y1) {
      ATOM_PANIC("gpu: Failed viewport validation: {} <= {}, {} <= {}\n", viewport.x0, viewport.x1, viewport.y0, viewport.y1);
    }

    for(const Polygon& polygon : polygons) {
      for(const Vertex* vertex : polygon.vertices) {
        const Vector4<Fixed20x12>& position = vertex->position;

        const i32 w = position.W().Raw();
        const i64 two_w = (i64)w << 1; // @todo: is using a 64-bit int accurate?

        const auto x = ((( (i64)position.X().Raw() + w) * viewport_width  + 0x800) / two_w) + viewport_x0;
        const auto y = (((-(i64)position.Y().Raw() + w) * viewport_height + 0x800) / two_w) + viewport_y0;

        if(x >= 0 && x < 256 && y >= 0 && y < 192) {
          m_frame_buffer[y][x] = Color4{63, 0, 0};
        }
      }
    }
  }

} // namespace dual::nds::gpu
