
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>

namespace dual::nds::gpu {

  void SoftwareRenderer::RenderEdgeMarking() {
    // @todo: the expanded clear depth is already calculated in "RenderRearPlane". Do not calculate it twice.
    const u32 border_depth = m_clear_depth;
    const u8 border_poly_id = m_io.clear_color.polygon_id;

    bool edge;

    const auto EvaluateCondition = [](u32 center_depth, u8 center_poly_id, u32 sample_depth, u8 sample_poly_id) {
      return sample_poly_id != center_poly_id && center_depth < sample_depth;
    };

    for(int y = 0; y < 192; y++) {
      for(int x = 0; x < 256; x++) {
        const PixelAttributes attributes = m_attribute_buffer[y][x];

        if(!(attributes.flags & PixelAttributes::Edge)) {
          continue;
        }

        const int x_l = x - 1;
        const int x_r = x + 1;
        const int y_u = y - 1;
        const int y_d = y + 1;

        const u32 center_depth  = m_depth_buffer[0][y][x];
        const u8 center_poly_id = attributes.poly_id[0];
        const bool border_edge = EvaluateCondition(center_depth, center_poly_id, border_depth, border_poly_id);

        edge  = x != 0   ? EvaluateCondition(center_depth, center_poly_id, m_depth_buffer[0][y][x_l], m_attribute_buffer[y][x_l].poly_id[0]) : border_edge;
        edge |= x != 255 ? EvaluateCondition(center_depth, center_poly_id, m_depth_buffer[0][y][x_r], m_attribute_buffer[y][x_r].poly_id[0]) : border_edge;
        edge |= y != 0   ? EvaluateCondition(center_depth, center_poly_id, m_depth_buffer[0][y_u][x], m_attribute_buffer[y_u][x].poly_id[0]) : border_edge;
        edge |= y != 191 ? EvaluateCondition(center_depth, center_poly_id, m_depth_buffer[0][y_d][x], m_attribute_buffer[y_d][x].poly_id[0]) : border_edge;

        if(edge) {
          const Color4 edge_color = m_edge_color[center_poly_id >> 3];

          m_frame_buffer[0][y][x].R() = edge_color.R();
          m_frame_buffer[0][y][x].G() = edge_color.G();
          m_frame_buffer[0][y][x].B() = edge_color.B();

          // @todo: It is unclear how edge-marking and anti-aliasing work together.
          if(m_io.disp3dcnt.enable_anti_aliasing) {
            m_coverage_buffer[y][x] = 32u;
          }
        }
      }
    }
  }

} // namespace dual::nds::gpu
