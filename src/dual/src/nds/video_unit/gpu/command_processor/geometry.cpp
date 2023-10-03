
#include <atom/panic.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdSetColor() {
    DequeueFIFO();
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

    m_geometry_engine.SubmitVertex({(i16)(u16)xy, (i16)(xy >> 16), (i16)(u16)z_});
  }

  void CommandProcessor::cmdSubmitVertex10() {
    const u32 xyz = DequeueFIFO();

    m_geometry_engine.SubmitVertex({(i16)(xyz << 6), (i16)(xyz >> 10 << 6), (i16)(xyz >> 20 << 6)});
  }

  void CommandProcessor::cmdSubmitVertexXY() {
    DequeueFIFO();

    ATOM_PANIC("gpu: Unimplemented VTX_XY");
  }

  void CommandProcessor::cmdSubmitVertexXZ() {
    ATOM_PANIC("gpu: Unimplemented VTX_XZ");
  }

  void CommandProcessor::cmdSubmitVertexYZ() {
    ATOM_PANIC("gpu: Unimplemented VTX_YZ");
  }

  void CommandProcessor::cmdSubmitVertexDelta() {
    ATOM_PANIC("gpu: Unimplemented VTX_DIFF");
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
