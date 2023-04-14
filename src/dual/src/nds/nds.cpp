
#include <dual/nds/nds.hpp>
#include <dual/nds/header.hpp>

namespace dual::nds {

  NDS::NDS() = default;

  void NDS::Reset() {
    m_memory.ewram.fill(0);
    m_memory.arm9.dtcm.fill(0);
    m_memory.arm9.itcm.fill(0);
  }

  void NDS::Step(int cycles) {
  }

  void NDS::LoadROM(std::shared_ptr<ROM> rom) {
    m_rom = std::move(rom);
  }

  void NDS::DirectBoot() {
    Header header{};

    if(m_rom->Size() < sizeof(Header)) {
      ATOM_PANIC("the loaded ROM is too small");
    }

    Reset();

    m_rom->Read(reinterpret_cast<u8*>(&header), 0, sizeof(Header));
  }

} // namespace dual::nds
