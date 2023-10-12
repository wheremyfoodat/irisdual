
#pragma once

#include <atom/vector_n.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>
#include <span>

namespace dual::nds::gpu {

  struct Vertex {
    Vector4<Fixed20x12> position;
    Vector2<Fixed12x4> uv;
    Color4 color;
  };

  struct Polygon {
    atom::Vector_N<Vertex*, 10> vertices;
  };

  class GeometryEngine {
    public:
      void Reset();
      void SwapBuffers();
      void Begin(u32 parameter);
      void End();
      void SubmitVertex(Vector3<Fixed20x12> position, const Matrix4<Fixed20x12>& clip_matrix);

    private:
      atom::Vector_N<Vertex, 10> ClipPolygon(const atom::Vector_N<Vertex, 10>& vertex_list, bool quad_strip);

      template<int axis, typename Comparator>
      bool ClipPolygonAgainstPlane(
        const atom::Vector_N<Vertex, 10>& vertex_list_in,
        atom::Vector_N<Vertex, 10>& vertex_list_out
      );

      atom::Vector_N<Vertex, 6144> m_vertex_ram[2];
      atom::Vector_N<Polygon, 2048> m_polygon_ram[2];
      atom::Vector_N<Vertex, 10> m_current_vertex_list;
      bool m_inside_vertex_list{};
      bool m_primitive_is_quad{};
      bool m_primitive_is_strip{};
      bool m_first_vertex{};
      int m_polygon_strip_length{};
      int m_current_buffer{};
  };

} // namespace dual::nds::gpu
