
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>
#include <limits>

#include "edge.hpp"
#include "interpolator.hpp"

namespace dual::nds::gpu {

  // @todo: move this into a header file.
  struct Span {
    i32 x0[2];
    i32 x1[2];
    Color4 color[2];
  };

  void SoftwareRenderer::RenderRearPlane() {
    for(int y = 0; y < 192; y++) {
      for(int x = 0; x < 256; x++) {
        m_frame_buffer[y][x] = Color4{4, 4, 4};
      }
    }
  }

  void SoftwareRenderer::RenderPolygons(const Viewport& viewport, std::span<const Polygon> polygons) {

#ifdef DEBUG_SLOPES
    const int dx = 69;
    const int dy = 49;
    const int ox = 256 / 2;
    const int oy = (192 - dy) / 2;

    Edge::Point a{ox, oy, 0, 0, nullptr};
    Edge::Point b{ox + dx, oy + dy, 0, 0, nullptr};
    Edge::Point c{ox - dx, oy + dy, 0, 0, nullptr};

    Edge edge1{a, b};
    Edge edge2{a, c};

    for(i32 y = a.y; y < b.y; y++) {
      i32 x0, x1;

      edge1.Interpolate(y, x0, x1);
      for(int x = x0>>18; x <= x1>>18; x++) {
        m_frame_buffer[y][x] = Color4{63, 0, 0};
      }

      edge2.Interpolate(y, x0, x1);
      for(int x = x0>>18; x <= x1>>18; x++) {
        m_frame_buffer[y][x] = Color4{63, 0, 0};
      }
    }

    return;
  #endif

    const int viewport_x0 = viewport.x0;
    const int viewport_y0 = viewport.y0;
    const int viewport_width = 1 + viewport.x1 - viewport_x0;
    const int viewport_height = 1 + viewport.y1 - viewport_y0;

    if(viewport.x0 > viewport.x1 || viewport.y0 > viewport.y1) {
      ATOM_PANIC("gpu: SW: failed viewport validation: {} <= {}, {} <= {}\n", viewport.x0, viewport.x1, viewport.y0, viewport.y1);
    }

    Span span{};
    Edge::Point points[10];
    Interpolator<9> edge_interp{};
    Interpolator<8> line_interp{};

    for(const Polygon& polygon : polygons) {
      const int vertex_count = (int)polygon.vertices.Size();

      bool should_render = true;

      int initial_vertex;
      int final_vertex;
      i32 y_min = std::numeric_limits<i32>::max();
      i32 y_max = std::numeric_limits<i32>::min();

      // @todo: move this part out of the renderer into the geometry engine.

      for(int i = 0; i < vertex_count; i++) {
        const Vertex* vertex = polygon.vertices[i];
        const Vector4<Fixed20x12>& position = vertex->position;

        // @todo: verify that using 64-bit integers is accurate
        const i64 w = position.W().Raw();
        const i64 two_w = (i64)w << 1;

        // @todo: this was not needed in the old codebase, maybe related to missing face culling?
        if(w == 0) {
          should_render = false;
          break;
        }

        const i32 x = (i32)(((( (i64)position.X().Raw() + w) * viewport_width  + 0x800) / two_w) + viewport_x0);
        const i32 y = (i32)((((-(i64)position.Y().Raw() + w) * viewport_height + 0x800) / two_w) + viewport_y0);
        const u32 depth = (u32)((((i64)position.Z().Raw() << 14) / w + 0x3FFF) << 9);

        points[i] = Edge::Point{x, y, depth, (i32)w, vertex};

        if(y < y_min) {
          y_min = y;
          initial_vertex = i;
        }

        if(y > y_max) {
          y_max = y;
          final_vertex = i;
        }
      }

      if(!should_render) {
        continue;
      }

      const int a = polygon.windedness <= 0 ? 0 : 1;
      const int b = a ^ 1;

      int start[2] { initial_vertex, initial_vertex };
      int end[2];

      end[a] = initial_vertex + 1;
      end[b] = initial_vertex - 1;

      if(end[a] == vertex_count) {
        end[a] = 0;
      }

      if(end[b] == -1) {
        end[b] = vertex_count - 1;
      }

      Edge edge[2] {
        {points[initial_vertex], points[end[0]]},
        {points[initial_vertex], points[end[1]]}
      };

      // Allow horizontal line polygons to render.
      if(y_min == y_max) y_max++;

      for(i32 y = y_min; y < y_max; y++) {
        if(y >= points[end[a]].y && end[a] != final_vertex) {
          do {
            start[a] = end[a];
            if(++end[a] == vertex_count) {
              end[a] = 0;
            }
          } while(y >= points[end[a]].y && end[a] != final_vertex);

          edge[a] = Edge{points[start[a]], points[end[a]]};
        }

        if(y >= points[end[b]].y && end[b] != final_vertex) {
          do {
            start[b] = end[b];
            if(--end[b] == -1) {
              end[b] = vertex_count - 1;
            }
          } while(y >= points[end[b]].y && end[b] != final_vertex);

          edge[b] = Edge{points[start[b]], points[end[b]]};
        }

        for(int i = 0; i < 2; i++) {
          edge[i].Interpolate(y, span.x0[i], span.x1[i]);
        }

        for(int i = 0; i < 2; i++) {
          const u16 w0 = polygon.w_16[start[i]];
          const u16 w1 = polygon.w_16[end[i]];

          if(edge[i].IsXMajor()) {
            const i32 x_min = points[start[i]].x;
            const i32 x_max = points[end[i]].x;
            // @todo: account for swapped left and right edges.
            const i32 x = (i == 0 ? span.x0[0] : span.x1[1]) >> 18;

            if(x_min <= x_max) {
              edge_interp.Setup(w0, w1, x, x_min, x_max);
            } else {
              edge_interp.Setup(w0, w1, (x_min - (x - x_max)), x_max, x_min);
            }
          } else {
            edge_interp.Setup(w0, w1, y, y_min, y_max);
          }

          edge_interp.Perp(points[start[i]].vertex->color, points[end[i]].vertex->color, span.color[i]);
        }

        if(y >= 0 && y < 192) {
          const bool force_render_inner_span = y == y_min || y == y_max - 1;

          const int xl0 = span.x0[0] >> 18;
          const int xl1 = span.x1[0] >> 18;
          const int xr0 = span.x0[1] >> 18;
          const int xr1 = span.x1[1] >> 18;

          for(int x = xl0; x <= xl1; x++) {
            if(x >= 0 && x < 256) {
              m_frame_buffer[y][x] = span.color[0];
            }
          }

          if(force_render_inner_span) {
            for(int x = xl1 + 1; x <= xr0 - 1; x++) {
              if(x >= 0 && x < 256) {
                m_frame_buffer[y][x] = Color4{0, 63, 0};
              }
            }
          }

          for(int x = xr0; x <= xr1; x++) {
            if(x >= 0 && x < 256) {
              m_frame_buffer[y][x] = span.color[1];
            }
          }
        }
      }
    }
  }

} // namespace dual::nds::gpu
