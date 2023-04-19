
#pragma once

#include <array>
#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <dual/nds/vram/region.hpp>

namespace dual::nds {

  struct VRAM {
    enum class Bank {
      A = 0,
      B = 1,
      C = 2,
      D = 3,
      E = 4,
      F = 5,
      G = 6,
      H = 7,
      I = 8
    };

    void Reset();

    u8    Read_VRAMSTAT();
    u8    Read_VRAMCNT(Bank bank);
    void Write_VRAMCNT(Bank bank, u8 value);

    // VRAM banks A - I (total: 656 KiB)
    std::array<u8, 0x20000> bank_a; // 128 KiB
    std::array<u8, 0x20000> bank_b; // 128 KiB
    std::array<u8, 0x20000> bank_c; // 128 KiB
    std::array<u8, 0x20000> bank_d; // 128 KiB
    std::array<u8, 0x10000> bank_e; //  64 KiB
    std::array<u8,  0x4000> bank_f; //  16 KiB
    std::array<u8,  0x4000> bank_g; //  16 KiB
    std::array<u8,  0x8000> bank_h; //  32 KiB
    std::array<u8,  0x4000> bank_i; //  16 KiB

    // VRAM regions
    Region<32> region_ppu_bg [2]{31, 7};            // PPU A and B background region
    Region<16> region_ppu_obj[2]{15, 7};            // PPU A and B object region
    Region<64> region_lcdc{63};                     // ARM9 direct access "LCDC" region
    Region<16> region_arm7_wram{15};                // ARM7 bank C and D region
    Region<4, 8192> region_ppu_bg_extpal [2]{3, 3}; // PPU A and B extended background palettes region
    Region<1, 8192> region_ppu_obj_extpal[2]{0, 0}; // PPU A and B extended object palettes regions
    Region<4, 131072> region_gpu_texture{3};        // GPU texture data region
    Region<8> region_gpu_palette{7};                // GPU palette region

  private:

    void UnmapA();
    void UnmapB();
    void UnmapC();
    void UnmapD();
    void UnmapE();
    void UnmapF();
    void UnmapG();
    void UnmapH();
    void UnmapI();

    void MapA();
    void MapB();
    void MapC();
    void MapD();
    void MapE();
    void MapF();
    void MapG();
    void MapH();
    void MapI();

    union VRAMCNT {
      atom::Bits<0, 3, u8> mst;
      atom::Bits<3, 2, u8> offset;
      atom::Bits<7, 1, u8> mapped;

      u8 byte = 0u;
    } m_vramcnt[9]{};
  };

} // namespace dual::nds
