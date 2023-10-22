
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
    const int viewport_x0 = viewport.x0;
    const int viewport_y0 = viewport.y0;
    const int viewport_width = 1 + viewport.x1 - viewport_x0;
    const int viewport_height = 1 + viewport.y1 - viewport_y0;

    if(viewport.x0 > viewport.x1 || viewport.y0 > viewport.y1) {
      ATOM_PANIC("gpu: Failed viewport validation: {} <= {}, {} <= {}\n", viewport.x0, viewport.x1, viewport.y0, viewport.y1);
    }

    Span span;
    Edge::Point points[10];

    for(const Polygon& polygon : polygons) {
      const int vertex_count = polygon.vertices.Size();

      bool should_render = true;

      int initial_vertex;
      i32 lowest_y = std::numeric_limits<i32>::max();
      i32 highest_y = std::numeric_limits<i32>::min();

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

        const i32 x = ((( (i64)position.X().Raw() + w) * viewport_width  + 0x800) / two_w) + viewport_x0;
        const i32 y = (((-(i64)position.Y().Raw() + w) * viewport_height + 0x800) / two_w) + viewport_y0;
        const u32 depth = (u32)((((i64)position.Z().Raw() << 14) / w + 0x3FFF) << 9);

//        if(x >= 0 && x < 256 && y >= 0 && y < 192) {
          //m_frame_buffer[y][x] = Color4{63, 0, 0};
//        }

        points[i] = Edge::Point{x, y, depth, (i32)w, vertex};

        if(y < lowest_y) { // @todo: confirm it is < and not <=
          lowest_y = y;
          initial_vertex = i;
        }

        if(y > highest_y) {
          highest_y = y;
        }
      }

      if(!should_render) {
        continue;
      }

      int start[2] { initial_vertex, initial_vertex };
      int end[2] { initial_vertex - 1, initial_vertex + 1 };

      if(end[0] == -1) {
        end[0] = vertex_count - 1;
      }

      if(end[1] == vertex_count) {
        end[1] = 0;
      }

      Edge edge[2] {
        {points[initial_vertex], points[end[0]]},
        {points[initial_vertex], points[end[1]]}
      };

      for(i32 y = lowest_y; y <= highest_y; y++) {
        for(int i = 0; i < 2; i++) {
          edge[i].Interpolate(y, span.x0[i], span.x1[i]);

          const int x0 = span.x0[i] >> 18;
          const int x1 = span.x1[i] >> 18;
          for(int x = x0; x <= x1; x++) {
            if(x >= 0 && x < 256 && y >= 0 && y < 192) {
              m_frame_buffer[y][x] = Color4{0, 32, 32};
            }
          }
        }

        if(y >= points[end[0]].y) {
          start[0] = end[0];
          if(--end[0] == -1) {
            end[0] = vertex_count - 1;
          }
          edge[0] = Edge{points[start[0]], points[end[0]]};
        }

        if(y >= points[end[1]].y) {
          start[1] = end[1];
          if(++end[1] == vertex_count) {
            end[1] = 0;
          }
          edge[1] = Edge{points[start[1]], points[end[1]]};
        }
      }
    }
  }

} // namespace dual::nds::gpu
