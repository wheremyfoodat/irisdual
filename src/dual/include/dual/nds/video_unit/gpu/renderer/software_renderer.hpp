
#pragma once

#include <array>
#include <atom/punning.hpp>
#include <dual/nds/video_unit/gpu/renderer/renderer_base.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>
#include <dual/nds/video_unit/gpu/registers.hpp>
#include <dual/nds/vram/region.hpp>

namespace dual::nds::gpu {

  class SoftwareRenderer final : public RendererBase {
    public:
      SoftwareRenderer(
        IO& io,
        const Region<4, 131072>& vram_texture,
        const Region<8>& vram_palette
      );

      void SetWBufferEnable(bool enable_w_buffer) {
        m_enable_w_buffer = enable_w_buffer;
      }

      void Render(const Viewport& viewport, std::span<const Polygon> polygons) override;

      void CaptureColor(int scanline, std::span<u16, 256> dst_buffer, int dst_width, bool display_capture) override;
      void CaptureAlpha(int scanline, std::span<int, 256> dst_buffer) override;

    private:
      void CopyVRAM();
      void RenderRearPlane();
      void RenderPolygons(const Viewport& viewport, std::span<const Polygon> polygons);
      void RenderPolygon(const Viewport& viewport, const Polygon& polygon);
      Color4 SampleTexture(TextureParams params, u32 palette_base, Vector2<Fixed12x4> uv);

      template<typename T>
      auto ReadTextureVRAM(u32 address) {
        return atom::read<T>(m_vram_texture_copy, address & 0x7FFFF & ~(sizeof(T) - 1));
      }

      template<typename T>
      auto ReadPaletteVRAM(u32 address) {
        return atom::read<T>(m_vram_palette_copy, address & 0x1FFFF & ~(sizeof(T) - 1));
      }

      IO& m_io;
      const Region<4, 131072>& m_vram_texture;
      const Region<8>& m_vram_palette;
      bool m_enable_w_buffer{};
      u8 m_vram_texture_copy[524288]{};
      u8 m_vram_palette_copy[131072]{};
      Color4 m_frame_buffer[192][256];
      u32 m_depth_buffer[192][256];
  };

} // namespace dual::nds::gpu
