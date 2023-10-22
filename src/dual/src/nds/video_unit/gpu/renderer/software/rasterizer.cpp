
#include <dual/nds/video_unit/gpu/renderer/software_renderer.hpp>
#include <limits>

#include "edge.hpp"

namespace dual::nds::gpu {

  // @todo: move this into a header file.
  struct Span {
    i32 x0[2];
    i32 x1[2];
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

    for(const Polygon& polygon : polygons) {
      const int vertex_count = (int)polygon.vertices.Size();

      bool should_render = true;

      int initial_vertex;
      int final_vertex;
      i32 y_min = std::numeric_limits<i32>::max();
      i32 y_max = std::numeric_limits<i32>::min();

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

      for(i32 y = y_min; y <= y_max; y++) {
        for(int i = 0; i < 2; i++) {
          edge[i].Interpolate(y, span.x0[i], span.x1[i]);

          const int x0 = span.x0[i] >> 18;
          const int x1 = span.x1[i] >> 18;

          for(int x = x0; x <= x1; x++) {
            if(x >= 0 && x < 256 && y >= 0 && y < 192) {
              m_frame_buffer[y][x] = i == 0 ? Color4{63, 0, 0} : Color4{0, 63, 0};//points[start[0]].vertex->color;
            }
          }
        }

        if(span.x0[0]-(1<<18) > span.x1[1]) {
          // this is probably related to inclusive vs exclusive ranges?
          //ATOM_PANIC("gpu: SW: detected swapped left and right edges in span: {} {}", span.x0[0]>>18, span.x1[1]>>18);
        }

        while(y >= points[end[a]].y - 1 && end[a] != final_vertex) {
          start[a] = end[a];
          if(++end[a] == vertex_count) {
            end[a] = 0;
          }
          edge[a] = Edge{points[start[a]], points[end[a]]};
        }

        while(y >= points[end[b]].y - 1 && end[b] != final_vertex) {
          start[b] = end[b];
          if(--end[b] == -1) {
            end[b] = vertex_count - 1;
          }
          edge[b] = Edge{points[start[b]], points[end[b]]};
        }
      }
    }
  }

} // namespace dual::nds::gpu
