
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

      Edge(const Point& p0, const Point& p1) : m_p0{&p0}, m_p1{&p1} {
        CalculateSlope();
      }

      [[nodiscard]] i32 GetXSlope() const {
        return m_x_slope;
      }

      [[nodiscard]] bool IsXMajor() const {
        return m_x_major;
      }

      void Interpolate(i32 y, i32& x0, i32& x1) {
        #ifndef NDEBUG
          if(y < m_p0->y || y > m_p1->y) {
            ATOM_PANIC("gpu: SW: y-coordinate {} was not in range [{}, {}] during edge interpolation", y, m_p0->y, m_p1->y);
          }
        #endif

        if(m_x_major) {
          // @todo: ensure that this is correct (particular for negative x-slopes)
          if(m_x_slope >= 0) {
            x0 = (m_p0->x << 18) + m_x_slope * (y - m_p0->y) + (1 << 17);

            if(y != m_p1->y || m_flat_horizontal) {
              x1 = (x0 & ~0x1FF) + m_x_slope - (1 << 18);
            } else {
              x1 = x0;
            }
          } else {
            x1 = (m_p0->x << 18) + m_x_slope * (y - m_p0->y) + (1 << 17);

            if(y != m_p1->y || m_flat_horizontal) {
              x0 = (x1 & ~0x1FF) + m_x_slope;
            } else {
              x0 = x1;
            }
          }
        } else {
          x0 = (m_p0->x << 18) + m_x_slope * (y - m_p0->y);
          x1 = x0;
        }
      }

    private:

      void CalculateSlope() {
        const i32 x_diff = m_p1->x - m_p0->x;
        const i32 y_diff = m_p1->y - m_p0->y;

        if(y_diff == 0) {
          m_x_slope = x_diff << 18;
          m_x_major = std::abs(x_diff) > 1;
          m_flat_horizontal = true;
        } else {
          const i32 y_reciprocal = (1 << 18) / y_diff;

          m_x_slope = x_diff * y_reciprocal;
          m_x_major = std::abs(x_diff) > std::abs(y_diff);
          m_flat_horizontal = false;
        }
      }

      const Point* m_p0;
      const Point* m_p1;
      i32 m_x_slope{};
      bool m_x_major{};
      bool m_flat_horizontal{};
  };

} // namespace dual::nds::gpu
