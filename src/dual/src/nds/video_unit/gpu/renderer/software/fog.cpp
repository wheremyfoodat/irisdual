
#include <algorithm>
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>

namespace dual::nds::gpu {

  void SoftwareRenderer::RenderFog() {
    Color4 fog_color;
    fog_color.R() = (i8)(m_io.fog_color.color_r << 1 | m_io.fog_color.color_r >> 4);
    fog_color.G() = (i8)(m_io.fog_color.color_g << 1 | m_io.fog_color.color_g >> 4);
    fog_color.B() = (i8)(m_io.fog_color.color_b << 1 | m_io.fog_color.color_b >> 4);
    fog_color.A() = (i8)(m_io.fog_color.color_a << 1 | m_io.fog_color.color_a >> 4);

    const auto ReadFogTable = [this](int index) {
      index = std::clamp(index, 0, 31);
      return (m_io.fog_table[index >> 2] >> ((index & 3) * 8)) & 0x7F;
    };

    const auto ApplyFog = [](Fixed6 a, Fixed6 b, int density) {
      if(density >= 128) return b.Raw();

      return (i8)((a.Raw() * (0x80 - density) + b.Raw() * density) >> 7);
    };

    const bool ignore_rgb = m_io.disp3dcnt.ignore_fog_rgb_color;

    const int fog_depth_shift = m_io.disp3dcnt.fog_depth_shift;

    for(int y = 0; y < 192; y++) {
      for(int x = 0; x < 256; x++) {
        if(!(m_attribute_buffer[y][x].flags & PixelAttributes::Fog)) {
          continue;
        }

        const u32 depth = m_depth_buffer[0][y][x];

        const i32 fog_table_index = ((i32)(depth >> 9) - (i32)m_io.fog_offset) << fog_depth_shift;
        const int fog_table_index_int   = fog_table_index >> 10;
        const int fog_table_index_fract = fog_table_index & 0x3FF;

        const int density0 = (int)ReadFogTable(fog_table_index_int);
        const int density1 = (int)ReadFogTable(fog_table_index_int + 1);
        const int density  = (density0 * (0x400 - fog_table_index_fract) + density1 * fog_table_index_fract) >> 10;

        if(ignore_rgb) {
          m_frame_buffer[0][y][x].A() = ApplyFog(m_frame_buffer[0][y][x].A(), fog_color.A(), density);
          m_frame_buffer[1][y][x].A() = ApplyFog(m_frame_buffer[1][y][x].A(), fog_color.A(), density);
        } else {
          for(const int i: {0, 1}) {
            Color4& target = m_frame_buffer[i][y][x];
            for(const int j: {0, 1, 2}) {
              target[j] = ApplyFog(target[j].Raw(), fog_color[j], density);
            }
          }
        }
      }
    }
  }

} // namespace dual::nds::gpu
