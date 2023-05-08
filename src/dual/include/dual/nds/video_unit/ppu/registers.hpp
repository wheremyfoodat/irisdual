
#include <atom/bit.hpp>
#include <atom/integer.hpp>

namespace dual::nds {

  struct DisplayControl {
    enum class Mapping : uint {
      TwoDimensional = 0,
      OneDimensional = 1
    };

    DisplayControl() : m_mask{0xFFFFFFFFu} {}

    explicit DisplayControl(u32 mask) : m_mask{mask} {}

    union {
      atom::Bits< 0, 3, u32> bg_mode;
      atom::Bits< 3, 1, u32> enable_bg0_3d;
      atom::Bits< 4, 1, u32> tile_obj_mapping;
      atom::Bits< 5, 1, u32> bitmap_obj_dimension; // ???
      atom::Bits< 6, 1, u32> bitmap_obj_mapping;
      atom::Bits< 7, 1, u32> forced_blank;
      atom::Bits< 8, 8, u32> enable; // @todo: rename this perhaps?
      atom::Bits<16, 2, u32> display_mode;
      atom::Bits<18, 2, u32> vram_block;
      atom::Bits<20, 2, u32> tile_obj_boundary; // ???
      atom::Bits<22, 1, u32> bitmap_obj_boundary; // ???
      atom::Bits<23, 1, u32> hblank_oam_update;
      atom::Bits<24, 3, u32> tile_block;
      atom::Bits<27, 3, u32> map_block;
      atom::Bits<30, 1, u32> enable_extpal_bg;
      atom::Bits<31, 1, u32> enable_extpal_obj;

      u32 word = 0u;
    };

    u32 ReadWord() {
      return word;
    }

    void WriteWord(u32 value, u32 mask) {
      const u32 write_mask = m_mask & mask;

      word = (value & write_mask) | (word & ~write_mask);
    }

    u32 m_mask;
  };

  struct BackgroundControl {
    union {
      atom::Bits< 0, 2, u16> priority;
      atom::Bits< 2, 4, u16> tile_block;
      atom::Bits< 6, 1, u16> enable_mosaic;
      atom::Bits< 7, 1, u16> full_palette;
      atom::Bits< 8, 5, u16> map_block;
      atom::Bits<13, 1, u16> palette_slot; // BG0-1
      atom::Bits<13, 1, u16> wraparound;   // BG2-3
      atom::Bits<14, 2, u16> size;

      u16 half = 0u;
    };

    u16 ReadHalf() {
      return half;
    }

    void WriteHalf(u16 value, u16 mask) {
      half = (value & mask) | (half & ~mask);
    }
  };

  struct BackgroundOffset {
    u16 half = 0u;

    u16 ReadHalf() {
      return half;
    }

    void WriteHalf(u16 value, u16 mask) {
      const u16 write_mask = 0x01FFu & mask;

      half = (value & write_mask) | (half & ~write_mask);
    }
  };

  struct ReferencePoint {
    u32 initial = 0u;
    s32 current = 0;

    u32 ReadWord() {
      return initial;
    }

    void WriteWord(u32 value, u32 mask) {
      initial = (value & mask & 0x0FFFFFFFu) | (initial & ~mask);

      if(initial & 0x08000000u) {
        initial |= 0xF0000000u;
      }

      current = (s32)initial;
    }
  };

  struct RotateScaleParameter {
    u16 half = 0u;

    void WriteHalf(u16 value, u16 mask) {
      half = (value & mask) | (half & ~mask);
    }
  };

  struct WindowRange {
    union {
      atom::Bits<0, 8, u16> max;
      atom::Bits<8, 8, u16> min;

      u16 half = 0u;
    };

    bool changed = true;

    void WriteHalf(u16 value, u16 mask) {
      half = (value & mask) | (half & ~mask);

      changed = true;
    }
  };

  struct WindowLayerSelect {
    union {
      atom::Bits<0, 6, u16> win0_layer_enable;
      atom::Bits<8, 6, u16> win1_layer_enable;

      u16 half = 0u;
    };

    u16 ReadHalf() {
      return half;
    }

    void WriteHalf(u16 value, u16 mask) {
      const u16 write_mask = 0x3F3Fu & mask;

      half = (value & write_mask) | (half & ~write_mask);
    }
  };

  struct BlendControl {
    enum class Mode : uint {
      Off = 0,
      Alpha = 1,
      Brighten = 2,
      Darken = 3
    };

    union {
      atom::Bits<0, 6, u16> dst_targets;
      atom::Bits<6, 2, u16> blend_mode;
      atom::Bits<8, 6, u16> src_targets;

      u16 half = 0u;
    };

    u16 ReadHalf() {
      return half;
    }

    void WriteHalf(u16 value, u16 mask) {
      const u16 write_mask = 0x3FFFu & mask;

      half = (value & write_mask) | (half & ~write_mask);
    }
  };

  struct BlendAlpha {
    union {
      atom::Bits<0, 5, u16> a;
      atom::Bits<8, 5, u16> b;

      u16 half = 0u;
    };

    u16 ReadHalf() {
      return half;
    }

    void WriteHalf(u16 value, u16 mask) {
      const u16 write_mask = 0x1F1Fu & mask;

      half = (value & write_mask) | (half & ~write_mask);
    }
  };

  struct BlendBrightness {
    u16 half = 0u;

    void WriteHalf(u16 value, u16 mask) {
      const u16 write_mask = 0x001Fu & mask;

      half = (value & write_mask) | (half & ~write_mask);
    }
  };

  struct Mosaic {
    struct {
      int size_x = 0;
      int size_y = 0;
      int counter_y = 0;
    } bg{}, obj{};

    void WriteHalf(u16 value, u16 mask) {
      if(mask & 0x00FFu) {
        bg.size_x = 1 + ((value >> 0) & 15);
        bg.size_y = 1 + ((value >> 4) & 15);
        bg.counter_y = 0;
      }

      if(mask & 0xFF00u) {
        obj.size_x = 1 + ((value >>  8) & 15);
        obj.size_y = 1 + ((value >> 12) & 15);
        obj.counter_y = 0;
      }
    }
  };

  struct MasterBrightness {
    enum class Mode {
        Off = 0,
        Up = 1,
        Down = 2,
        Reserved = 3
    };

    union {
      atom::Bits< 0, 5, u16> factor;
      atom::Bits<14, 2, u16> mode;

      u16 half = 0u;
    };

    u16 ReadHalf() {
      return half;
    }

    void WriteHalf(u16 value, u16 mask) {
      const u16 write_mask = 0xC01Fu & mask;

      half = (value & write_mask) | (half & ~write_mask);
    }
  };

} // namespace dual::nds
