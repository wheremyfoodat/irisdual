
#pragma once

#include <atom/integer.hpp>
#include <cstdlib>
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>

namespace dual::nds::gpu {

  class Edge {
    public:
      struct Point {
        i32 x;
        i32 y;
        u32 depth;
        i32 w;
        const Vertex* vertex;
      };

      Edge(const Point& p0, const Point& p1) : m_y_start{p0.y} {
        const i32 x_diff = p1.x - p0.x;
        const i32 y_diff = p1.y - p0.y;

        if(y_diff == 0) {
          m_x_slope = x_diff << 18;
          m_x_major = std::abs(x_diff) > 1;
        } else {
          const i32 y_reciprocal = (1 << 18) / y_diff;

          m_x_slope = x_diff * y_reciprocal;
          m_x_major = std::abs(x_diff) > std::abs(y_diff);
        }

        m_x_start = p0.x << 18;

        if(m_x_major || std::abs(x_diff) == std::abs(y_diff)) {
          if(m_x_slope > 0) {
            m_x_start += 1 << 17;
          } else {
            m_x_start -= 1 << 17;
          }
        }
      }

      [[nodiscard]] bool IsXMajor() const {
        return m_x_major;
      }

      [[nodiscard]] i32 GetXSlope() const {
        return m_x_slope;
      }

      void Interpolate(i32 y, i32& x0, i32& x1) const {
        if(m_x_major) {
          if(m_x_slope > 0) {
            x0 = m_x_start + m_x_slope * (y - m_y_start);
            x1 = (x0 & ~0x1FF) + m_x_slope - (1 << 18);
          } else {
            x1 = m_x_start + m_x_slope * (y - m_y_start);
            x0 = (x1 |  0x1FF) + m_x_slope + (1 << 18);
          }
        } else {
          x0 = m_x_start + m_x_slope * (y - m_y_start);
          x1 = x0;
        }
      }

    private:
      i32 m_y_start;
      i32 m_x_start{};
      i32 m_x_slope{};
      bool m_x_major{};
  };

} // namespace dual::nds::gpu
