
#include <atom/panic.hpp>
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdSetColor() {
    m_geometry_engine.SetVertexColor(Color4::FromRGB555((u16)DequeueFIFO()));
  }

  void CommandProcessor::cmdSetNormal() {
    const u32 xyz = DequeueFIFO();

    m_geometry_engine.SetNormal((m_direction_mtx * Vector4<Fixed20x12>{
      (i32)(xyz & (0x3FFu <<  0)) << 22 >> 19,
      (i32)(xyz & (0x3FFu << 10)) << 12 >> 19,
      (i32)(xyz & (0x3FFu << 20)) <<  2 >> 19,
      0
    }).XYZ(), m_texture_mtx);
  }

  void CommandProcessor::cmdSetUV() {
    const u32 st = (u32)DequeueFIFO();
    const Fixed12x4 s = (i16)(u16)st;
    const Fixed12x4 t = (i16)(st >> 16);

    m_geometry_engine.SetVertexUV({s, t}, m_texture_mtx);
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

  void CommandProcessor::cmdSetPolygonAttributes() {
    m_geometry_engine.SetPolygonAttributes((u32)DequeueFIFO());
  }

  void CommandProcessor::cmdSetTextureParameters() {
    m_geometry_engine.SetTextureParameters(DequeueFIFO());
  }

  void CommandProcessor::cmdSetPaletteBase() {
    m_geometry_engine.SetPaletteBase(DequeueFIFO() & 0x1FFFu);
  }

  void CommandProcessor::cmdSetDiffuseAndAmbientMaterialColors() {
    const u32 param = DequeueFIFO();
    const Color4 diffuse = Color4::FromRGB555((u16)(param >>  0));
    const Color4 ambient = Color4::FromRGB555((u16)(param >> 16));

    m_geometry_engine.SetMaterialAmbientColor(ambient);
    m_geometry_engine.SetMaterialDiffuseColor(diffuse);
    if(param & 0x8000u) {
      m_geometry_engine.SetVertexColor(diffuse);
    }
  }

  void CommandProcessor::cmdSetSpecularAndEmissiveMaterialColors() {
    const u32 param = DequeueFIFO();
    const Color4 specular = Color4::FromRGB555((u16)(param >>  0));
    const Color4 emissive = Color4::FromRGB555((u16)(param >> 16));

    m_geometry_engine.SetMaterialSpecularColor(specular);
    m_geometry_engine.SetMaterialEmissiveColor(emissive);
    m_geometry_engine.SetShininessTableEnable(param & 0x8000u);
  }

  void CommandProcessor::cmdSetLightVector() {
    const u32 i_xyz = DequeueFIFO();

    const int light_index = i_xyz >> 30;

    const Vector4<Fixed20x12> light_direction = m_direction_mtx * Vector4<Fixed20x12>{
      (i32)(i_xyz & (0x3FFu <<  0)) << 22 >> 19,
      (i32)(i_xyz & (0x3FFu << 10)) << 12 >> 19,
      (i32)(i_xyz & (0x3FFu << 20)) <<  2 >> 19,
      0
    };

    m_geometry_engine.SetLightDirection(light_index, light_direction.XYZ());
  }

  void CommandProcessor::cmdSetLightColor() {
    const u32 i_rgb555 = DequeueFIFO();

    m_geometry_engine.SetLightColor((int)(i_rgb555 >> 30), Color4::FromRGB555((u16)i_rgb555));
  }

  void CommandProcessor::cmdSetShininessTable() {
    std::array<u8, 128> shininess_table = m_geometry_engine.GetShininessTable();

    for(int a = 0; a < 128; a += 4) {
      u32 word = (u32)DequeueFIFO();

      for(int b = 0; b < 4; b++) {
        shininess_table[a | b] = (u8)word;
        word >>= 8;
      }
    }
  }

} // namespace dual::nds::gpu
