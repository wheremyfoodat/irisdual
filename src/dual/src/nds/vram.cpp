
#include <atom/logger/logger.hpp>
#include <dual/nds/vram/vram.hpp>

namespace dual::nds {

  void VRAM::Reset() {
    bank_a.fill(0);
    bank_b.fill(0);
    bank_c.fill(0);
    bank_d.fill(0);
    bank_e.fill(0);
    bank_f.fill(0);
    bank_g.fill(0);
    bank_h.fill(0);
    bank_i.fill(0);

    for(auto bank : {0, 1, 2, 3, 4, 5, 6, 7, 8}) Write_VRAMCNT((Bank)bank, 0u);
  }

  u8 VRAM::Read_VRAMSTAT() {
    auto const& vramcnt_c = m_vramcnt[(int)Bank::C];
    auto const& vramcnt_d = m_vramcnt[(int)Bank::D];

    return (vramcnt_c.mapped && vramcnt_c.mst == 2 ? 1u : 0u) |
           (vramcnt_d.mapped && vramcnt_d.mst == 2 ? 2u : 0u);
  }

  u8 VRAM::Read_VRAMCNT(Bank bank) {
    return m_vramcnt[(int)bank].byte;
  }

  void VRAM::Write_VRAMCNT(Bank bank, u8 value) {
    auto& vramcnt = m_vramcnt[(int)bank];

    if(vramcnt.mapped) {
      switch(bank) {
        case Bank::A: UnmapA(); break;
        case Bank::B: UnmapB(); break;
        case Bank::C: UnmapC(); break;
        case Bank::D: UnmapD(); break;
        case Bank::E: UnmapE(); break;
        case Bank::F: UnmapF(); break;
        case Bank::G: UnmapG(); break;
        case Bank::H: UnmapH(); break;
        case Bank::I: UnmapI(); break;
      }
    }

    vramcnt.byte = value & 0x9Fu;

    if(vramcnt.mapped) {
      switch(bank) {
        case Bank::A: MapA(); break;
        case Bank::B: MapB(); break;
        case Bank::C: MapC(); break;
        case Bank::D: MapD(); break;
        case Bank::E: MapE(); break;
        case Bank::F: MapF(); break;
        case Bank::G: MapG(); break;
        case Bank::H: MapH(); break;
        case Bank::I: MapI(); break;
      }
    }
  }

  void VRAM::UnmapA() {
    auto const& vramcnt_a = m_vramcnt[(int)Bank::A];

    switch (vramcnt_a.mst) {
      case 0: region_lcdc.Unmap(0x00000, bank_a); break;
      case 1: region_ppu_bg[0].Unmap(0x20000 * vramcnt_a.offset, bank_a); break;
      case 2: region_ppu_obj[0].Unmap(0x20000 * (vramcnt_a.offset & 1), bank_a); break;
      case 3: region_gpu_texture.Unmap(vramcnt_a.offset * 0x20000, bank_a); break;
    }
  }

  void VRAM::UnmapB() {
    auto const& vramcnt_b = m_vramcnt[(int)Bank::B];

    switch (vramcnt_b.mst) {
      case 0: region_lcdc.Unmap(0x20000, bank_b); break;
      case 1: region_ppu_bg[0].Unmap(0x20000 * vramcnt_b.offset, bank_b); break;
      case 2: region_ppu_obj[0].Unmap(0x20000 * (vramcnt_b.offset & 1), bank_b); break;
      case 3: region_gpu_texture.Unmap(vramcnt_b.offset * 0x20000, bank_b); break;
    }
  }

  void VRAM::UnmapC() {
    auto const& vramcnt_c = m_vramcnt[(int)Bank::C];

    switch (vramcnt_c.mst) {
      case 0: region_lcdc.Unmap(0x40000, bank_c); break;
      case 1: region_ppu_bg[0].Unmap(0x20000 * vramcnt_c.offset, bank_c); break;
      case 2: region_arm7_wram.Unmap(0x20000 * (vramcnt_c.offset & 1), bank_c); break;
      case 3: region_gpu_texture.Unmap(vramcnt_c.offset * 0x20000, bank_c); break;
      case 4: region_ppu_bg[1].Unmap(0x00000, bank_c); break;
    }
  }

  void VRAM::UnmapD() {
    auto const& vramcnt_d = m_vramcnt[(int)Bank::D];

    switch (vramcnt_d.mst) {
      case 0: region_lcdc.Unmap(0x60000, bank_d); break;
      case 1: region_ppu_bg[0].Unmap(0x20000 * vramcnt_d.offset, bank_d); break;
      case 2: region_arm7_wram.Unmap(0x20000 * (vramcnt_d.offset & 1), bank_d); break;
      case 3: region_gpu_texture.Unmap(vramcnt_d.offset * 0x20000, bank_d); break;
      case 4: region_ppu_obj[1].Unmap(0x00000, bank_d); break;
    }
  }

  void VRAM::UnmapE() {
    auto const& vramcnt_e = m_vramcnt[(int)Bank::E];

    switch (vramcnt_e.mst) {
      case 0: region_lcdc.Unmap(0x80000, bank_e); break;
      case 1: region_ppu_bg[0].Unmap(0x00000, bank_e); break;
      case 2: region_ppu_obj[0].Unmap(0x00000, bank_e); break;
      case 3: region_gpu_palette.Unmap(0, bank_e); break;
      case 4: region_ppu_bg_extpal[0].Unmap(0, bank_e, 0x8000); break;
    }
  }

  void VRAM::UnmapF() {
    auto const& vramcnt_f = m_vramcnt[(int)Bank::F];

    switch (vramcnt_f.mst) {
      case 0: region_lcdc.Unmap(0x90000, bank_f); break;
      case 1: region_ppu_bg[0].Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f); break;
      case 2: region_ppu_obj[0].Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f); break;
      case 3: region_gpu_palette.Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f); break;
      case 4: region_ppu_bg_extpal[0].Unmap(0x4000 * (vramcnt_f.offset & 1), bank_f); break;
      case 5: region_ppu_obj_extpal[0].Unmap(0, bank_f, 0x2000); break;
    }
  }

  void VRAM::UnmapG() {
    auto const& vramcnt_g = m_vramcnt[(int)Bank::G];

    switch (vramcnt_g.mst) {
      case 0: region_lcdc.Unmap(0x94000, bank_g); break;
      case 1: region_ppu_bg[0].Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g); break;
      case 2: region_ppu_obj[0].Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g); break;
      case 3: region_gpu_palette.Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g); break;
      case 4: region_ppu_bg_extpal[0].Unmap(0x4000 * (vramcnt_g.offset & 1), bank_g); break;
      case 5: region_ppu_obj_extpal[0].Unmap(0, bank_g, 0x2000); break;
    }
  }

  void VRAM::UnmapH() {
    auto const& vramcnt_h = m_vramcnt[(int)Bank::H];

    switch (vramcnt_h.mst) {
      case 0: region_lcdc.Unmap(0x98000, bank_h); break;
      case 1: region_ppu_bg[1].Unmap(0x00000, bank_h); break;
      case 2: region_ppu_bg_extpal[1].Unmap(0, bank_h); break;
    }
  }

  void VRAM::UnmapI() {
    auto const& vramcnt_i = m_vramcnt[(int)Bank::I];

    switch (vramcnt_i.mst) {
      case 0: region_lcdc.Unmap(0xA0000, bank_i); break;
      case 1: region_ppu_bg[1].Unmap(0x08000, bank_i); break;
      case 2: region_ppu_obj[1].Unmap(0x00000, bank_i); break;
      case 3: region_ppu_obj_extpal[1].Unmap(0, bank_i, 0x2000); break;
    }
  }

  void VRAM::MapA() {
    auto const& vramcnt_a = m_vramcnt[(int)Bank::A];

    switch (vramcnt_a.mst) {
      case 0: region_lcdc.Map(0x00000, bank_a); break;
      case 1: region_ppu_bg[0].Map(0x20000 * vramcnt_a.offset, bank_a); break;
      case 2: region_ppu_obj[0].Map(0x20000 * (vramcnt_a.offset & 1), bank_a); break;
      case 3: region_gpu_texture.Map(vramcnt_a.offset * 0x20000, bank_a); break;
      default: ATOM_ERROR("VRAM bank A: unsupported configuration: mst={0} offset={1}", vramcnt_a.mst, vramcnt_a.offset);
    }
  }

  void VRAM::MapB() {
    auto const& vramcnt_b = m_vramcnt[(int)Bank::B];

    switch (vramcnt_b.mst) {
      case 0: region_lcdc.Map(0x20000, bank_b); break;
      case 1: region_ppu_bg[0].Map(0x20000 * vramcnt_b.offset, bank_b); break;
      case 2: region_ppu_obj[0].Map(0x20000 * (vramcnt_b.offset & 1), bank_b); break;
      case 3: region_gpu_texture.Map(vramcnt_b.offset * 0x20000, bank_b); break;
      default: ATOM_ERROR("VRAM bank B: unsupported configuration: mst={0} offset={1}", vramcnt_b.mst, vramcnt_b.offset);
    }
  }

  void VRAM::MapC() {
    auto const& vramcnt_c = m_vramcnt[(int)Bank::C];

    switch (vramcnt_c.mst) {
      case 0: region_lcdc.Map(0x40000, bank_c); break;
      case 1: region_ppu_bg[0].Map(0x20000 * vramcnt_c.offset, bank_c); break;
      case 2: region_arm7_wram.Map(0x20000 * (vramcnt_c.offset & 1), bank_c); break;
      case 3: region_gpu_texture.Map(vramcnt_c.offset * 0x20000, bank_c); break;
      case 4: region_ppu_bg[1].Map(0x00000, bank_c); break;
      default: ATOM_ERROR("VRAM bank C: unsupported configuration: mst={0} offset={1}", vramcnt_c.mst, vramcnt_c.offset);
    }
  }

  void VRAM::MapD() {
    auto const& vramcnt_d = m_vramcnt[(int)Bank::D];

    switch (vramcnt_d.mst) {
      case 0: region_lcdc.Map(0x60000, bank_d); break;
      case 1: region_ppu_bg[0].Map(0x20000 * vramcnt_d.offset, bank_d); break;
      case 2: region_arm7_wram.Map(0x20000 * (vramcnt_d.offset & 1), bank_d); break;
      case 3: region_gpu_texture.Map(vramcnt_d.offset * 0x20000, bank_d); break;
      case 4: region_ppu_obj[1].Map(0x00000, bank_d); break;
      default: ATOM_ERROR("VRAM bank D: unsupported configuration: mst={0} offset={1}", vramcnt_d.mst, vramcnt_d.offset);
    }
  }

  void VRAM::MapE() {
    auto const& vramcnt_e = m_vramcnt[(int)Bank::E];

    switch (vramcnt_e.mst) {
      case 0: region_lcdc.Map(0x80000, bank_e); break;
      case 1: region_ppu_bg[0].Map(0x00000, bank_e); break;
      case 2: region_ppu_obj[0].Map(0x00000, bank_e); break;
      case 3: region_gpu_palette.Map(0, bank_e); break;
      case 4: region_ppu_bg_extpal[0].Map(0, bank_e, 0x8000); break;
      default: ATOM_ERROR("VRAM bank E: unsupported configuration: mst={0} offset={1}", vramcnt_e.mst, vramcnt_e.offset);
    }
  }

  void VRAM::MapF() {
    auto const& vramcnt_f = m_vramcnt[(int)Bank::F];

    switch (vramcnt_f.mst) {
      case 0: region_lcdc.Map(0x90000, bank_f); break;
      case 1: region_ppu_bg[0].Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f); break;
      case 2: region_ppu_obj[0].Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f); break;
      case 3: region_gpu_palette.Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f); break;
      case 4: region_ppu_bg_extpal[0].Map(0x4000 * (vramcnt_f.offset & 1), bank_f); break;
      case 5: region_ppu_obj_extpal[0].Map(0, bank_f, 0x2000); break;
      default: ATOM_ERROR("VRAM bank F: unsupported configuration: mst={0} offset={1}", vramcnt_f.mst, vramcnt_f.offset);
    }
  }

  void VRAM::MapG() {
    auto const& vramcnt_g = m_vramcnt[(int)Bank::G];

    switch (vramcnt_g.mst) {
      case 0: region_lcdc.Map(0x94000, bank_g); break;
      case 1: region_ppu_bg[0].Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g); break;
      case 2: region_ppu_obj[0].Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g); break;
      case 3: region_gpu_palette.Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g); break;
      case 4: region_ppu_bg_extpal[0].Map(0x4000 * (vramcnt_g.offset & 1), bank_g); break;
      case 5: region_ppu_obj_extpal[0].Map(0, bank_g, 0x2000); break;
      default: ATOM_ERROR("VRAM bank G: unsupported configuration: mst={0} offset={1}", vramcnt_g.mst, vramcnt_g.offset);
    }
  }

  void VRAM::MapH() {
    auto const& vramcnt_h = m_vramcnt[(int)Bank::H];

    switch (vramcnt_h.mst) {
      case 0: region_lcdc.Map(0x98000, bank_h); break;
      case 1: region_ppu_bg[1].Map(0x00000, bank_h); break;
      case 2: region_ppu_bg_extpal[1].Map(0, bank_h); break;
      default: ATOM_ERROR("VRAM bank H: unsupported configuration: mst={0} offset={1}", vramcnt_h.mst, vramcnt_h.offset);
    }
  }

  void VRAM::MapI() {
    auto const& vramcnt_i = m_vramcnt[(int)Bank::I];

    switch (vramcnt_i.mst) {
      case 0: region_lcdc.Map(0xA0000, bank_i); break;
      case 1: region_ppu_bg[1].Map(0x08000, bank_i); break;
      case 2: region_ppu_obj[1].Map(0x00000, bank_i); break;
      case 3: region_ppu_obj_extpal[1].Map(0, bank_i, 0x2000); break;
      default: ATOM_ERROR("VRAM bank I: unsupported configuration: mst={0} offset={1}", vramcnt_i.mst, vramcnt_i.offset);
    }
  }

} // namespace dual::nds
