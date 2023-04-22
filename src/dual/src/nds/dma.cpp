
#include <dual/nds/dma.hpp>

namespace dual::nds {

  static constexpr int k_address_offset[4] { 1, -1, 0, 1 };

  using Bus = arm::Memory::Bus;

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

      latch.sad = m_dmasad[id];
      latch.dad = m_dmadad[id];

      if(dmacnt.length == 0u) {
        latch.length = 0x200000u;
      } else {
        latch.length = dmacnt.length;
      }

      if(dmacnt.timing == StartTime::Immediate) {
        Run(id);
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
    auto& dmacnt = m_dmacnt[id];
    auto& latch = m_latch[id];

    const int sad_offset = k_address_offset[dmacnt.src_address_mode] << (1 + dmacnt.transfer_size);
    const int dad_offset = k_address_offset[dmacnt.dst_address_mode] << (1 + dmacnt.transfer_size);

    while(latch.length > 0u) {
      switch(dmacnt.transfer_size) {
        case 0u: m_bus.WriteHalf(latch.dad, m_bus.ReadHalf(latch.sad, Bus::System), Bus::System);
        case 1u: m_bus.WriteWord(latch.dad, m_bus.ReadWord(latch.sad, Bus::System), Bus::System);
      }

      latch.sad += sad_offset;
      latch.dad += dad_offset;
      latch.length--;
    }

    if(dmacnt.repeat && dmacnt.timing != StartTime::Immediate) {
      if(dmacnt.dst_address_mode == 3u) {
        latch.dad = m_dmadad[id];
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
      m_irq.Raise((IRQ::Source)((u32)IRQ::Source::DMA0 << id));
    }
  }

} // namespace dual::nds
