
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>

namespace dual::nds::gpu {

  void GeometryEngine::Reset() {
    m_last_position = {};

    for(auto& ram : m_vertex_ram) ram.Clear();
    for(auto& ram : m_polygon_ram) ram.Clear();
  }

  void GeometryEngine::SubmitVertex(Vector3<Fixed20x12> position) {
    // ...
  }

} // namespace dual::nds::gpu
