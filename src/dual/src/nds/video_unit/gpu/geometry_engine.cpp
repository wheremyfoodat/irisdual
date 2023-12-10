
#include <algorithm>
#include <atom/logger/logger.hpp>
#include <atom/float.hpp>
#include <atom/panic.hpp>
#include <bit>
#include <cmath>
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>
#include <limits>

namespace dual::nds::gpu {

  GeometryEngine::GeometryEngine(gpu::IO& io) : m_io{io} {
  }

  void GeometryEngine::Reset() {
    for(auto& ram : m_vertex_ram) ram.Clear();
    for(auto& ram : m_polygon_ram) ram.Clear();

    m_inside_vertex_list = false;
    m_primitive_is_quad = false;
    m_primitive_is_strip = false;
    m_first_vertex = false;
    m_polygon_strip_length = 0;
    m_current_buffer = 1;
    m_pending_polygon_attributes = 0u;
    m_polygon_attributes = {};
    m_vertex_color = {};
    m_vertex_uv = {};
    m_texture_parameters = {};
    m_light_direction = {};
    m_light_color = {};
    m_material_diffuse_color = {};
    m_material_ambient_color = {};
    m_material_specular_color = {};
    m_material_emissive_color = {};
  }

  void GeometryEngine::SwapBuffers() {
    m_current_buffer ^= 1;

    m_vertex_ram[m_current_buffer].Clear();
    m_polygon_ram[m_current_buffer].Clear();
  }

  void GeometryEngine::Begin(u32 parameter) {
    m_inside_vertex_list = true;
    m_primitive_is_quad = parameter & 1;
    m_primitive_is_strip = parameter & 2;
    m_first_vertex = true;
    m_polygon_strip_length = 0;
    m_current_vertex_list.Clear();

    m_polygon_attributes.word = m_pending_polygon_attributes;
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

    m_current_vertex_list.PushBack({clip_position, m_vertex_uv, m_vertex_color});

    int required_vertex_count = m_primitive_is_quad ? 4 : 3;

    if(m_primitive_is_strip && !m_first_vertex) {
      required_vertex_count -= 2;
    }

    if(m_current_vertex_list.Size() != required_vertex_count) {
      return;
    }

    atom::Vector_N<Polygon, 2048>& poly_ram = m_polygon_ram[m_current_buffer];
    atom::Vector_N<Vertex,  6144>& vert_ram = m_vertex_ram[m_current_buffer];

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

    int windedness;
    
    if(poly.vertices.Size() == 2) {
      windedness = CalculateWindedness(
        poly.vertices[0]->position,
        m_current_vertex_list.Back().position,
        poly.vertices[1]->position
      );
    } else {
      windedness = CalculateWindedness(
        m_current_vertex_list[0].position,
        m_current_vertex_list.Back().position,
        m_current_vertex_list[1].position
      );
    }

    if(invert_winding) {
      windedness = -windedness;
    }

    const bool cull = !(
      windedness == 0 ||
      (m_polygon_attributes.render_front_face && windedness < 0) ||
      (m_polygon_attributes.render_back_face  && windedness > 0)
    );

    if(cull) {
      if(m_primitive_is_strip) {
        m_polygon_strip_length++;
      }

      if(needs_clipping && m_primitive_is_strip) {
        m_current_vertex_list.Erase(m_current_vertex_list.begin());
        if(m_primitive_is_quad) {
          m_current_vertex_list.Erase(m_current_vertex_list.begin());
        }

        m_first_vertex = true;
      } else {
        if(m_primitive_is_strip) {
          const int size = (int)m_current_vertex_list.Size();

          for(int i = std::max(0, size - 2); i < size; i++) {
            // @todo: how does this behave in real hardware?
            if(vert_ram.Full()) {
              ATOM_ERROR("gpu: Submitted more vertices than fit into Vertex RAM.");
              m_io.disp3dcnt.poly_or_vert_ram_overflow = true;
              return;
            }

            vert_ram.PushBack(m_current_vertex_list[i]);
          }

          m_first_vertex = false;
        }

        m_current_vertex_list.Clear();
      }

      return;
    }

    if(needs_clipping) {
      // Keep the last two unclipped vertices to restart the polygon strip.
      if(m_primitive_is_strip) {
        next_vertex_list.PushBack(m_current_vertex_list[m_current_vertex_list.Size() - 2]);
        next_vertex_list.PushBack(m_current_vertex_list[m_current_vertex_list.Size() - 1]);
      }

      m_current_vertex_list = ClipPolygon(
        m_current_vertex_list,
        m_primitive_is_quad && m_primitive_is_strip,
        m_polygon_attributes.render_far_plane_intersecting
      );
    }

    for(const Vertex& v : m_current_vertex_list) {
      // @todo: how does this behave in real hardware?
      if(vert_ram.Full()) {
        ATOM_ERROR("gpu: Submitted more vertices than fit into Vertex RAM.");
        m_io.disp3dcnt.poly_or_vert_ram_overflow = true;
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
      // @todo: calculate sorting key
      // @todo: supposedly texture parameters cannot be changed within polygon strips.
      poly.attributes = m_polygon_attributes;
      poly.texture_params.word = m_texture_parameters;
      poly.palette_base = m_texture_palette_base;
      poly.windedness = windedness;

      NormalizeW(poly);

      // @todo: this is not ideal in terms of performance
      if(!poly_ram.Full()) [[likely]] {
        poly_ram.PushBack(poly);
      } else {
        m_io.disp3dcnt.poly_or_vert_ram_overflow = true;
      }
    }
  }

  void GeometryEngine::NormalizeW(Polygon& poly) {
    int min_leading = 32;

    for(const Vertex* vertex : poly.vertices) {
      const i32 w = vertex->position.W().Raw();

      if(w < 0) {
        min_leading = std::min(min_leading, std::countl_one((u32)w));
      } else {
        min_leading = std::min(min_leading, std::countl_zero((u32)w));
      }
    }

    if(min_leading < 16) {
      int w_shift = 16 - min_leading;

      if(w_shift & 3) {
        w_shift = (w_shift | 3) + 1;
      }

      for(const Vertex* vertex : poly.vertices) {
        poly.w_16.PushBack(vertex->position.W().Raw() >> w_shift);
      }

      poly.w_l_shift = w_shift;
      poly.w_r_shift = 0;
    } else {
      const int w_shift = (min_leading & ~3) - 16;

      for(const Vertex* vertex : poly.vertices) {
        poly.w_16.PushBack(vertex->position.W().Raw() << w_shift);
      }

      poly.w_l_shift = 0;
      poly.w_r_shift = w_shift;
    }
  }

  atom::Vector_N<Vertex, 10> GeometryEngine::ClipPolygon(
    const atom::Vector_N<Vertex, 10>& vertex_list,
    bool quad_strip,
    bool render_far_plane_intersecting
  ) {
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

    const bool far_plane_intersecting = ClipPolygonAgainstPlane<2, CompareGt>(clipped[0], clipped[1]);
    if(!render_far_plane_intersecting && far_plane_intersecting) {
      // @todo: test if this is actually working as intended!
      return {};
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
    const int clip_precision = 18;

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
            const int sign = v0.position[axis] < -v0.position.W() ? 1 : -1;
            const i64 numer = v1.position[axis].Raw() + sign * v1.position[3].Raw();
            const i64 denom = (v0.position.W() - v1.position.W()).Raw() + (v0.position[axis] - v1.position[axis]).Raw() * sign;
            const i64 scale = -sign * ((i64)numer << clip_precision) / denom;
            const i64 scale_inv = (1 << clip_precision) - scale;

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

  int GeometryEngine::CalculateWindedness(
    const Vector4<Fixed20x12>& v0,
    const Vector4<Fixed20x12>& v1,
    const Vector4<Fixed20x12>& v2
  ) {
    const f32 a[3] {
      (f32)(v1.X() - v0.X()).Raw() / (f32)(1 << 12),
      (f32)(v1.Y() - v0.Y()).Raw() / (f32)(1 << 12),
      (f32)(v1.W() - v0.W()).Raw() / (f32)(1 << 12)
    };

    const f32 b[3] {
      (f32)(v2.X() - v0.X()).Raw() / (f32)(1 << 12),
      (f32)(v2.Y() - v0.Y()).Raw() / (f32)(1 << 12),
      (f32)(v2.W() - v0.W()).Raw() / (f32)(1 << 12)
    };

    const f32 normal[3] {
      a[1] * b[2] - a[2] * b[1],
      a[2] * b[0] - a[0] * b[2],
      a[0] * b[1] - a[1] * b[0]
    };

    const f32 dot = ((f32)v0.X().Raw() / (f32)(1 << 12)) * normal[0] +
                    ((f32)v0.Y().Raw() / (f32)(1 << 12)) * normal[1] +
                    ((f32)v0.W().Raw() / (f32)(1 << 12)) * normal[2];

    if(std::abs(dot) < std::numeric_limits<f32>::epsilon()) {
      // @todo: ensure that this check is sufficient to detect lines.
      return 0;
    }
    return dot > 0 ? +1 : -1;
  }

  void GeometryEngine::SetNormal(Vector3<Fixed20x12> normal) {
    // @todo: texture mapping

    i32 r = m_material_emissive_color.R().Raw() << 18;
    i32 g = m_material_emissive_color.G().Raw() << 18;
    i32 b = m_material_emissive_color.B().Raw() << 18;

    const Color4 diffuse  = m_material_diffuse_color;
    const Color4 ambient  = m_material_ambient_color;
    const Color4 specular = m_material_specular_color;

    for(int i = 0; i < 4; i++) {
      if(!m_polygon_attributes.light_enable[i]) {
        continue;
      }

      const Vector3<Fixed20x12> halfway{
        m_light_direction[i].X().Raw() >> 1,
        m_light_direction[i].Y().Raw() >> 1,
       (m_light_direction[i].Z().Raw() - (1 << 12)) >> 1
      };

      const i32 n_dot_l = std::max<i32>(0, -normal.Dot(m_light_direction[i]).Raw());
      const i32 n_dot_h = std::max<i32>(0, -normal.Dot(halfway).Raw());
      const i32 n_dot_h_2 = (n_dot_h * n_dot_h) >> 12;

      // @todo: implement shininess table

      r += m_light_color[i].R().Raw() * (n_dot_l * diffuse.R().Raw() + n_dot_h_2 * specular.R().Raw() + ambient.R().Raw());
      g += m_light_color[i].G().Raw() * (n_dot_l * diffuse.G().Raw() + n_dot_h_2 * specular.G().Raw() + ambient.G().Raw());
      b += m_light_color[i].B().Raw() * (n_dot_l * diffuse.B().Raw() + n_dot_h_2 * specular.B().Raw() + ambient.B().Raw());
    }

    // @todo: getting some random colors which are off. figure out why this happens.
    m_vertex_color.R() = std::min(r >> 18, 63);
    m_vertex_color.G() = std::min(g >> 18, 63);
    m_vertex_color.B() = std::min(b >> 18, 63);
  }

} // namespace dual::nds::gpu
