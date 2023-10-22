
#include <atom/panic.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdSetColor() {
    m_geometry_engine.SetVertexColor(Color4::FromRGB555((u16)DequeueFIFO()));
  }

  void CommandProcessor::cmdSetNormal() {
    DequeueFIFO();
  }

  void CommandProcessor::cmdSetUV() {
    DequeueFIFO();
  }

  void CommandProcessor::cmdSubmitVertex16() {
    const u32 xy = DequeueFIFO();
    const u32 z_ = DequeueFIFO();

    SubmitVertex({(i16)(u16)xy, (i16)(xy >> 16), (i16)(u16)z_});
  }

  void CommandProcessor::cmdSubmitVertex10() {
    const u32 xyz = DequeueFIFO();

    SubmitVertex({(i16)(xyz << 6), (i16)(xyz >> 10 << 6), (i16)(xyz >> 20 << 6)});
  }

  void CommandProcessor::cmdSubmitVertexXY() {
    const u32 xy = DequeueFIFO();

    SubmitVertex({(i16)(u16)xy, (i16)(xy >> 16), m_last_position.Z()});
  }

  void CommandProcessor::cmdSubmitVertexXZ() {
    const u32 xz = DequeueFIFO();

    SubmitVertex({(i16)(u16)xz, m_last_position.Y(), (i16)(xz >> 16)});
  }

  void CommandProcessor::cmdSubmitVertexYZ() {
    const u32 yz = DequeueFIFO();

    SubmitVertex({m_last_position.X(), (i16)(u16)yz, (i16)(yz >> 16)});
  }

  void CommandProcessor::cmdSubmitVertexDelta() {
    const u32 xyz_diff = DequeueFIFO();

    SubmitVertex({
      m_last_position.X() + ((i32)((xyz_diff & (0x3FFu <<  0)) << 22) >> 22),
      m_last_position.Y() + ((i32)((xyz_diff & (0x3FFu << 10)) << 12) >> 22),
      m_last_position.Z() + ((i32)((xyz_diff & (0x3FFu << 20)) <<  2) >> 22)
    });
  }

  void CommandProcessor::cmdSetPolygonAttrs() {
    DequeueFIFO();
  }

  void CommandProcessor::cmdSetTextureAttrs() {
    DequeueFIFO();
  }

  void CommandProcessor::cmdSetPaletteBase() {
    DequeueFIFO();
  }

} // namespace dual::nds::gpu
