
#include <dual/nds/arm9/dma.hpp>

namespace dual::nds::arm9 {

  void DMA::Reset() {
    for(auto& dmasad : m_dmasad) dmasad = 0u;
    for(auto& dmadad : m_dmadad) dmadad = 0u;
    for(auto& dmacnt : m_dmacnt) dmacnt = {};
    for(auto& dmafill : m_dmafill) dmafill = 0u;
    for(auto& latch : m_latch) latch = {};
  }

  void DMA::Request(StartTime timing) {
    for(int id : {0, 1, 2, 3}) {
      const auto& dmacnt = m_dmacnt[id];

      if(dmacnt.enable && dmacnt.timing == timing) {
        Run(id);
      }
    }
  }

  u32 DMA::Read_DMASAD(int id) {
    return m_dmasad[id];
  }

  void DMA::Write_DMASAD(int id, u32 value, u32 mask) {
    const u32 write_mask = 0x0FFFFFFFu & mask;

    m_dmasad[id] = (value & write_mask) | (m_dmasad[id] & ~write_mask);
  }

  u32 DMA::Read_DMADAD(int id) {
    return m_dmadad[id];
  }

  void DMA::Write_DMADAD(int id, u32 value, u32 mask) {
    const u32 write_mask = 0x0FFFFFFFu & mask;

    m_dmadad[id] = (value & write_mask) | (m_dmadad[id] & ~write_mask);
  }

  u32 DMA::Read_DMACNT(int id) {
    return m_dmacnt[id].word;
  }

  void DMA::Write_DMACNT(int id, u32 value, u32 mask) {
    auto& dmacnt = m_dmacnt[id];

    const bool old_enable = dmacnt.enable;

    dmacnt.word = (value & mask) | (dmacnt.word & ~mask);

    if(!old_enable && dmacnt.enable) {
      auto& latch = m_latch[id];

      if(dmacnt.transfer_32bits) {
        latch.sad = m_dmasad[id] & ~3u;
        latch.dad = m_dmadad[id] & ~3u;
      } else {
        latch.sad = m_dmasad[id] & ~1u;
        latch.dad = m_dmadad[id] & ~1u;
      }

      if(dmacnt.length == 0u) {
        latch.length = 0x200000u;
      } else {
        latch.length = dmacnt.length;
      }

      if(dmacnt.timing == StartTime::Immediate) {
        Run(id);
      }

      if(dmacnt.timing == StartTime::GXFIFO) {
        ATOM_PANIC("arm9: DMA: unimplemented GXFIFO DMA enabled.");
      }
    }
  }

  u32 DMA::Read_DMAFILL(int id) {
    return m_dmafill[id];
  }

  void DMA::Write_DMAFILL(int id, u32 value, u32 mask) {
    m_dmafill[id] = (value & mask) | (m_dmafill[id] & ~mask);
  }

  void DMA::Run(int id) {
    static constexpr int k_address_offset[2][4] {
      {2, -2, 0, 2},
      {4, -4, 0, 4}
    };

    using Bus = arm::Memory::Bus;

    auto& dmacnt = m_dmacnt[id];
    auto& latch = m_latch[id];

    const int sad_offset = k_address_offset[dmacnt.transfer_32bits][dmacnt.src_address_mode];
    const int dad_offset = k_address_offset[dmacnt.transfer_32bits][dmacnt.dst_address_mode];

    if(dmacnt.transfer_32bits) {
      while(latch.length-- > 0u) {
        m_bus.WriteWord(latch.dad, m_bus.ReadWord(latch.sad, Bus::System), Bus::System);

        latch.sad += sad_offset;
        latch.dad += dad_offset;
      }
    } else {
      while(latch.length-- > 0u) {
        m_bus.WriteHalf(latch.dad, m_bus.ReadHalf(latch.sad, Bus::System), Bus::System);

        latch.sad += sad_offset;
        latch.dad += dad_offset;
      }
    }

    if(dmacnt.repeat && dmacnt.timing != StartTime::Immediate) {
      if(dmacnt.dst_address_mode == 3u) {
        switch(dmacnt.transfer_32bits) {
          case 0u: latch.dad = m_dmadad[id] & ~1u; break;
          case 1u: latch.dad = m_dmadad[id] & ~3u; break;
        }
      }

      if(dmacnt.length == 0u) {
        latch.length = 0x200000u;
      } else {
        latch.length = dmacnt.length;
      }
    } else {
      dmacnt.enable = false;
    }

    if(dmacnt.enable_irq) {
      m_irq.Request((IRQ::Source)((u32)IRQ::Source::DMA0 << id));
    }
  }

} // namespace dual::nds::arm9
