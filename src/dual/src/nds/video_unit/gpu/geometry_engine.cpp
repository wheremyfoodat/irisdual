
#include <atom/logger/logger.hpp>
#include <atom/float.hpp>
#include <atom/panic.hpp>
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>

namespace dual::nds::gpu {

  void GeometryEngine::Reset() {
    for(auto& ram : m_vertex_ram) ram.Clear();
    for(auto& ram : m_polygon_ram) ram.Clear();

    m_inside_vertex_list = false;
    m_primitive_is_quad = false;
    m_primitive_is_strip = false;
    m_first_vertex = false;
    m_polygon_strip_length = 0;
  }

  void GeometryEngine::Begin(u32 parameter) {
    m_inside_vertex_list = true;
    m_primitive_is_quad = parameter & 1;
    m_primitive_is_strip = parameter & 2;
    m_first_vertex = true; // @todo: check this for correctness
    m_polygon_strip_length = 0; // @todo: check this for correctness
  }

  void GeometryEngine::End() {
    // @todo: supposedly this is a no-operation in real hardware.
    m_inside_vertex_list = false;
  }

  void GeometryEngine::SubmitVertex(Vector3<Fixed20x12> position, const Matrix4<Fixed20x12>& clip_matrix) {
    if(!m_inside_vertex_list) {
      ATOM_PANIC("gpu: Submitted a vertex outside of a vertex list.");
    }

    // @todo: texture mapping

    const Vector4<Fixed20x12> clip_position = clip_matrix * Vector4<Fixed20x12>{position};

    if(m_current_vertex_list.Full()) {
      return; // @todo: better handle this
    }

    // @todo: use proper vertex color and UV:
    m_current_vertex_list.PushBack({clip_position});

    int required_vertex_count = m_primitive_is_quad ? 4 : 3;

    if(m_primitive_is_strip && !m_first_vertex) {
      required_vertex_count -= 2;
    }

    if(m_current_vertex_list.Size() != required_vertex_count) {
      return;
    }

    // @todo: use the correct buffer for the current frame
    atom::Vector_N<Polygon, 2048>& poly_ram = m_polygon_ram[0];
    atom::Vector_N<Vertex,  6144>& vert_ram = m_vertex_ram[0];

    if(poly_ram.Size() == 2048) {
      return;
    }

    Polygon poly;

    bool needs_clipping = false;

    // Determine if the polygon needs to be clipped.
    for(const Vertex& v : m_current_vertex_list) {
      const Fixed20x12 w = v.position.W();

      for(int i = 0; i < 3; i++) {
        if(v.position[i] < -w || v.position[i] > w) {
          needs_clipping = true;
          break;
        }
      }
    }

    // In a triangle strip the winding order of each second polygon is inverted.
    const bool invert_winding = m_primitive_is_strip && !m_primitive_is_quad && (m_polygon_strip_length % 2) == 1;

    atom::Vector_N<Vertex, 10> next_vertex_list{};

    poly.vertices.Clear();

    // Append the last two vertices from vertex RAM to the polygon
    if(m_primitive_is_strip && !m_first_vertex) {
      if(needs_clipping) {
        // @todo: can we do this more efficiently?
        m_current_vertex_list.Insert(m_current_vertex_list.begin(), vert_ram[vert_ram.Size() - 1]);
        m_current_vertex_list.Insert(m_current_vertex_list.begin(), vert_ram[vert_ram.Size() - 2]);
      } else {
        poly.vertices.PushBack(&vert_ram[vert_ram.Size() - 2]);
        poly.vertices.PushBack(&vert_ram[vert_ram.Size() - 1]);
      }
    }

    // @todo: face culling
    const bool cull = false;

    if(cull) {
      // ...
      return;
    }

    if(needs_clipping) {
      // Keep the last two unclipped vertices to restart the polygon strip.
      if(m_primitive_is_strip) {
        next_vertex_list.PushBack(m_current_vertex_list[m_current_vertex_list.Size() - 2]);
        next_vertex_list.PushBack(m_current_vertex_list[m_current_vertex_list.Size() - 1]);
      }

      m_current_vertex_list = ClipPolygon(m_current_vertex_list, m_primitive_is_quad && m_primitive_is_strip);
    }

    for(const Vertex& v : m_current_vertex_list) {
      // @todo: how does this behave in real hardware?
      if(vert_ram.Size() == 6144) {
        ATOM_ERROR("gpu: Submitted more vertices than fit into Vertex RAM.");
        return;
      }

      vert_ram.PushBack(v);
      poly.vertices.PushBack(&vert_ram.Back());
    }

    // ClipPolygon() will have already swapped the vertices.
    if(!needs_clipping) {
      /**
       * v0---v2     v0---v3
       * |     | --> |     |
       * v1---v3     v1---v2
       */
      if(m_primitive_is_quad && m_primitive_is_strip) {
        std::swap(poly.vertices[2], poly.vertices[3]);
      }
    }

    // Setup state for the next polygon.
    if(needs_clipping && m_primitive_is_strip) {
      // Restart the polygon strip based on the last two submitted vertices
      m_first_vertex = true;
      m_current_vertex_list = next_vertex_list;
    } else {
      m_first_vertex = false;
      m_current_vertex_list.Clear();
    }

    if(m_primitive_is_strip) {
      m_polygon_strip_length++;
    }

    if(!poly.vertices.Empty()) {
      // @todo: copy polygon params, texture params and calculate sorting key

      // @todo: this is not ideal in terms of performance
      poly_ram.PushBack(poly);
    }
  }

  atom::Vector_N<Vertex, 10> GeometryEngine::ClipPolygon(const atom::Vector_N<Vertex, 10>& vertex_list, bool quad_strip) {
    atom::Vector_N<Vertex, 10> clipped[2];

    clipped[0] = vertex_list;

    if(quad_strip) {
      std::swap(clipped[0][2], clipped[0][3]);
    }

    struct CompareLt {
      bool operator()(Fixed20x12 x, Fixed20x12 w) { return x < -w; }
    };

    struct CompareGt {
      bool operator()(Fixed20x12 x, Fixed20x12 w) { return x >  w; }
    };

    // @todo: get this parameter from the polygon parameters
    const bool render_far_plane_polys = true;

    if(!render_far_plane_polys && ClipPolygonAgainstPlane<2, CompareGt>(clipped[0], clipped[1])) {
      return {}; // @todo: test if this is actually working as intended!
    }
    clipped[0].Clear();
    ClipPolygonAgainstPlane<2, CompareLt>(clipped[1], clipped[0]);
    clipped[1].Clear();

    ClipPolygonAgainstPlane<1, CompareGt>(clipped[0], clipped[1]);
    clipped[0].Clear();
    ClipPolygonAgainstPlane<1, CompareLt>(clipped[1], clipped[0]);
    clipped[1].Clear();

    ClipPolygonAgainstPlane<0, CompareGt>(clipped[0], clipped[1]);
    clipped[0].Clear();
    ClipPolygonAgainstPlane<0, CompareLt>(clipped[1], clipped[0]);

    return clipped[0];
  }

  template<int axis, typename Comparator>
  bool GeometryEngine::ClipPolygonAgainstPlane(
    const atom::Vector_N<Vertex, 10>& vertex_list_in,
    atom::Vector_N<Vertex, 10>& vertex_list_out
  ) {
    const int clip_precision = 10;

    const int size = (int)vertex_list_in.Size();

    bool clipped = false;

    for(int i = 0; i < size; i++) {
      const Vertex& v0 = vertex_list_in[i];

      if(Comparator{}(v0.position[axis], v0.position.W())) {
        int a = i - 1;
        int b = i + 1;

        if(a == -1) a = size -1;
        if(b == size) b = 0;

        for(int j : {a, b}) {
          const Vertex& v1 = vertex_list_in[j];

          if(!Comparator{}(v1.position[axis], v1.position.W())) {
            // @todo: remove usage of 'auto' keyword
            const auto sign = v0.position[axis] < -v0.position.W() ? 1 : -1;
            const auto numer = v1.position[axis].Raw() + sign * v1.position[3].Raw();
            const auto denom = (v0.position.W() - v1.position.W()).Raw() + (v0.position[axis] - v1.position[axis]).Raw() * sign;
            const auto scale = -sign * ((i64)numer << clip_precision) / denom;
            const auto scale_inv = (1 << clip_precision) - scale;

            Vertex clipped_vertex{};

            for(const int k : {0, 1, 2, 3}) {
              clipped_vertex.position[k] = (v1.position[k].Raw() * scale_inv + v0.position[k].Raw() * scale) >> clip_precision;
              clipped_vertex.color[k] = (v1.color[k].Raw() * scale_inv + v0.color[k].Raw() * scale) >> clip_precision;
            }

            for(const int k : {0, 1}) {
              clipped_vertex.uv[k] = (v1.uv[k].Raw() * scale_inv + v0.uv[k].Raw() * scale) >> clip_precision;
            }

            vertex_list_out.PushBack(clipped_vertex);
          }
        }

        clipped = true;
      } else {
        vertex_list_out.PushBack(v0);
      }
    }

    return clipped;
  }

} // namespace dual::nds::gpu
