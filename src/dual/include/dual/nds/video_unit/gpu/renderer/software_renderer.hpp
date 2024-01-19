
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

      void SetWBufferEnable(bool enable_w_buffer) override {
        m_enable_w_buffer = enable_w_buffer;
      }

      virtual void UpdateToonTable(size_t table_offset, std::span<const u32> table_data) override {
        size_t i = table_offset * 2u;

        for(u32 pair : table_data) {
          m_toon_table[i++] = Color4::FromRGB555((u16)(pair >>  0));
          m_toon_table[i++] = Color4::FromRGB555((u16)(pair >> 16));
        }
      }

      void Render(const Viewport& viewport, std::span<const Polygon* const> polygons) override;

      void CaptureColor(int scanline, std::span<u16, 256> dst_buffer, int dst_width, bool display_capture) override;
      void CaptureAlpha(int scanline, std::span<int, 256> dst_buffer) override;

    private:
      struct Line {
        int x[2];
        Color4 color[2];
        Vector2<Fixed12x4> uv[2];
        u16 w_16[2];
        u32 depth[2];
      };

      struct PixelAttributes {
        enum Flags : u16 {
          Shadow = 1,
          Edge = 2
        };

        u8 poly_id[2];
        u16 flags;
      };

      void CopyVRAM();
      void RenderRearPlane();
      void RenderPolygons(const Viewport& viewport, std::span<const Polygon* const> polygons);
      void RenderPolygon(const Viewport& viewport, const Polygon& polygon);
      void RenderPolygonSpan(const Polygon& polygon, const Line& line, i32 y, int x0, int x1);
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
      PixelAttributes m_attribute_buffer[192][256];
      std::array<Color4, 32> m_toon_table{};
  };

} // namespace dual::nds::gpu
