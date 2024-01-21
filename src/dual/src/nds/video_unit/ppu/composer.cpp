
#include <dual/nds/video_unit/ppu/ppu.hpp>

namespace dual::nds {

  template<bool window, bool blending, bool opengl>
  void PPU::ComposeScanlineTmpl(u16 vcount, int bg_min, int bg_max) {
    auto& mmio = m_mmio_copy[vcount];

    u16 backdrop = ReadPalette(0, 0);

    const auto& dispcnt = mmio.dispcnt;
    const auto& bgcnt = mmio.bgcnt;
    const auto& winin = mmio.winin;
    const auto& winout = mmio.winout;

    bool bg0_is_3d = mmio.dispcnt.enable_bg0_3d || mmio.dispcnt.bg_mode == 6;

    int bg_list[4];
    int bg_count = 0;

    if constexpr(opengl) {
      // 3D BG0 layer will be handled on the GPU
      if(bg_min == 0 && bg0_is_3d) {
        bg_min = 1;
      }
    }

    // Sort enabled backgrounds by their respective priority in ascending order.
    for(int prio = 3; prio >= 0; prio--) {
      for(int bg = bg_max; bg >= bg_min; bg--) {
        if(dispcnt.enable[bg] && bgcnt[bg].priority == prio) {
          bg_list[bg_count++] = bg;
        }
      }
    }

    bool win0_active = false;
    bool win1_active = false;
    bool win2_active = false;

    atom::Bits<0, 6, u8> win_layer_enable{};

    if constexpr (window) {
      win0_active = dispcnt.enable[ENABLE_WIN0] && m_window_scanline_enable[0];
      win1_active = dispcnt.enable[ENABLE_WIN1] && m_window_scanline_enable[1];
      win2_active = dispcnt.enable[ENABLE_OBJWIN];
    }

    int prio[2];
    int layer[2];
    u16 pixel[2];

    size_t buffer_index = 256 * vcount;

    for(int x = 0; x < 256; x++) {
      if constexpr (window) {
        // Determine the window with the highest priority for this pixel.
        if(win0_active && m_buffer_win[0][x]) {
          win_layer_enable = (u8)winin.win0_layer_enable;
        } else if(win1_active && m_buffer_win[1][x]) {
          win_layer_enable = (u8)winin.win1_layer_enable;
        } else if(win2_active && m_buffer_obj[x].window) {
          win_layer_enable = (u8)winout.win1_layer_enable;
        } else {
          win_layer_enable = (u8)winout.win0_layer_enable;
        }
      }

      if constexpr (blending) {
        bool is_alpha_obj = false;

        prio[0] = 4;
        prio[1] = 4;
        layer[0] = LAYER_BD;
        layer[1] = LAYER_BD;

        // Find up to two top-most visible background pixels.
        for(int i = 0; i < bg_count; i++) {
          int bg = bg_list[i];

          if(!window || win_layer_enable[bg]) {
            auto pixel_new = m_buffer_bg[bg][x];
            if(pixel_new != k_color_transparent) {
              layer[1] = layer[0];
              layer[0] = bg;
              prio[1] = prio[0];
              prio[0] = bgcnt[bg].priority;
            }
          }
        }

        /* Check if a OBJ pixel takes priority over one of the two
         * top-most background pixels and insert it accordingly.
         */
        if((!window || win_layer_enable[LAYER_OBJ]) &&
            dispcnt.enable[ENABLE_OBJ] &&
            m_buffer_obj[x].color != k_color_transparent) {
          int priority = m_buffer_obj[x].priority;

          if(priority <= prio[0]) {
            layer[1] = layer[0];
            layer[0] = LAYER_OBJ;
            is_alpha_obj = m_buffer_obj[x].alpha;

            if constexpr(opengl) {
              prio[0] = priority;
            }
          } else if(priority <= prio[1]) {
            layer[1] = LAYER_OBJ;

            if constexpr(opengl) {
              prio[1] = priority;
            }
          }
        }

        // Map layer numbers to pixels.
        for(int i = 0; i < 2; i++) {
          int _layer = layer[i];

          switch(_layer) {
            case 0: case 1:
            case 2: case 3:
              pixel[i] = m_buffer_bg[_layer][x];
              break;
            case 4:
              pixel[i] = m_buffer_obj[x].color;
              break;
            case 5:
              pixel[i] = backdrop;
              break;
          }
        }

        auto sfx_enable = !window || win_layer_enable[LAYER_SFX];

        if constexpr(opengl) {
          // buffer_ogl_color[0][buffer_index] = ConvertColor(pixel[0]);
          // buffer_ogl_color[1][buffer_index] = ConvertColor(pixel[1]);
          //
          // buffer_ogl_attribute[buffer_index] = (sfx_enable   ? 0x8000 : 0) |
          //                                      (is_alpha_obj ? 0x4000 : 0) |
          //                                      (prio[1] << 8) |
          //                                      (prio[0] << 6) |
          //                                      (layer[1] << 3) |
          //                                       layer[0];
        } else {
          auto blend_mode = mmio.bldcnt.blend_mode;
          bool have_dst = mmio.bldcnt.dst_targets[layer[0]];
          bool have_src = mmio.bldcnt.src_targets[layer[1]];

          if(is_alpha_obj && have_src) {
            pixel[0] = AlphaBlend(pixel[0], pixel[1], (int)mmio.bldalpha.a, (int)mmio.bldalpha.b);
          } if(blend_mode == BlendControl::Mode::Alpha) {
            // @todo: what does HW do if "enable BG0 3D" is disabled in mode 6.
            if(layer[0] == 0 && bg0_is_3d && have_src) {
              const int eva = m_buffer_3d_alpha[x];
              const int evb = 16 - eva;

              pixel[0] = AlphaBlend(pixel[0], pixel[1], eva, evb);
            } else if(have_dst && have_src && sfx_enable) {
              pixel[0] = AlphaBlend(pixel[0], pixel[1], (int)mmio.bldalpha.a, (int)mmio.bldalpha.b);
            }
          } else if(blend_mode != BlendControl::Mode::Off) {
            if(have_dst && sfx_enable) {
              switch((BlendControl::Mode)blend_mode) {
                case BlendControl::Mode::Brighten:
                  pixel[0] = Brighten(pixel[0], (int)mmio.bldy.half);
                  break;
                case BlendControl::Mode::Darken:
                  pixel[0] = Darken(pixel[0], (int)mmio.bldy.half);
                  break;
                default: break;
              }
            }
          }
        }
      } else {
        pixel[0] = backdrop;
        prio[0] = 4;
        layer[0] = LAYER_BD;

        // Find the top-most visible background pixel.
        if(bg_count != 0) {
          for(int i = bg_count - 1; i >= 0; i--) {
            int bg = bg_list[i];

            if(!window || win_layer_enable[bg]) {
              u16 pixel_new = m_buffer_bg[bg][x];
              if(pixel_new != k_color_transparent) {
                pixel[0] = pixel_new;
                layer[0] = bg;
                prio[0] = bgcnt[bg].priority;
                break;
              }
            }
          }
        }

        // Check if a OBJ pixel takes priority over the top-most background pixel.
        if((!window || win_layer_enable[LAYER_OBJ]) &&
            dispcnt.enable[ENABLE_OBJ] &&
            m_buffer_obj[x].color != k_color_transparent &&
            m_buffer_obj[x].priority <= prio[0]) {
          pixel[0] = m_buffer_obj[x].color;
          if constexpr(opengl) {
            layer[0] = LAYER_OBJ;
            prio[0] = m_buffer_obj[x].priority;
          }
        }

        if constexpr(opengl) {
          // buffer_ogl_color[0][buffer_index] = ConvertColor(pixel[0]);
          // buffer_ogl_attribute[buffer_index] = (prio[0] << 6) | (LAYER_BD << 3) | layer[0];
        }
      }

      if constexpr(!opengl) {
        m_buffer_compose[x] = pixel[0] | 0x8000;
      }

      buffer_index++;
    }
  }

  void PPU::ComposeScanline(u16 vcount, int bg_min, int bg_max) {
    const auto& mmio = m_mmio_copy[vcount];
    const auto& dispcnt = mmio.dispcnt;

    int key = 0;

    if(dispcnt.enable[ENABLE_WIN0] ||
        dispcnt.enable[ENABLE_WIN1] ||
        dispcnt.enable[ENABLE_OBJWIN]) {
      key |= 1;
    }

    if(mmio.bldcnt.blend_mode != BlendControl::Mode::Off || m_line_contains_alpha_obj) {
      key |= 2;
    }

    /*if(ogl.enabled) {
      key |= 4;
    }*/

    switch(key) {
      case 0b000: ComposeScanlineTmpl<false, false, false>(vcount, bg_min, bg_max); break;
      case 0b001: ComposeScanlineTmpl<true,  false, false>(vcount, bg_min, bg_max); break;
      case 0b010: ComposeScanlineTmpl<false, true,  false>(vcount, bg_min, bg_max); break;
      case 0b011: ComposeScanlineTmpl<true,  true,  false>(vcount, bg_min, bg_max); break;
      case 0b100: ComposeScanlineTmpl<false, false, true >(vcount, bg_min, bg_max); break;
      case 0b101: ComposeScanlineTmpl<true,  false, true >(vcount, bg_min, bg_max); break;
      case 0b110: ComposeScanlineTmpl<false, true,  true >(vcount, bg_min, bg_max); break;
      case 0b111: ComposeScanlineTmpl<true,  true,  true >(vcount, bg_min, bg_max); break;
    }
  }

  u16 PPU::AlphaBlend(u16 color_a, u16 color_b, int eva, int evb) {
    const int r_a = (color_a >>  0) & 0x1F;
    const int g_a = (color_a >>  5) & 0x1F;
    const int b_a = (color_a >> 10) & 0x1F;

    const int r_b = (color_b >>  0) & 0x1F;
    const int g_b = (color_b >>  5) & 0x1F;
    const int b_b = (color_b >> 10) & 0x1F;

    eva = std::min<int>(16, eva);
    evb = std::min<int>(16, evb);

    return std::min<u8>((r_a * eva + r_b * evb) >> 4, 31) |
           std::min<u8>((g_a * eva + g_b * evb) >> 4, 31) <<  5 |
           std::min<u8>((b_a * eva + b_b * evb) >> 4, 31) << 10;
  }

  u16 PPU::Brighten(u16 color, int evy) {
    const int r = (color >>  0) & 0x1F;
    const int g = (color >>  5) & 0x1F;
    const int b = (color >> 10) & 0x1F;

    evy = std::min<int>(16, evy);

    return (r + (((31 - r) * evy) >> 4)) |
           (g + (((31 - g) * evy) >> 4)) <<  5 |
           (b + (((31 - b) * evy) >> 4)) << 10;
  }

  u16 PPU::Darken(u16 color, int evy) {
    const int r = (color >>  0) & 0x1F;
    const int g = (color >>  5) & 0x1F;
    const int b = (color >> 10) & 0x1F;

    evy = std::min<int>(16, evy);

    return (r - ((r * evy) >> 4)) |
           (g - ((g * evy) >> 4)) <<  5 |
           (b - ((b * evy) >> 4)) << 10;
  }

} // namespace nds::nds
