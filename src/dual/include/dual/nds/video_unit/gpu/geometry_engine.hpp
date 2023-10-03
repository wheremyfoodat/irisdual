
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
      void SubmitVertex(Vector3<Fixed20x12> position);

    private:
      Vector3<Fixed20x12> m_last_position;
      atom::Vector_N<Vertex, 6144> m_vertex_ram[2];
      atom::Vector_N<Polygon, 2048> m_polygon_ram[2];
  };

} // namespace dual::nds::gpu
