
#include <atom/punning.hpp>
#include <dual/nds/nds.hpp>
#include <dual/nds/header.hpp>

namespace dual::nds {

  NDS::NDS() = default;

  void NDS::Reset() {
    m_memory.ewram.fill(0);
    m_memory.arm9.dtcm.fill(0);
    m_memory.arm9.itcm.fill(0);
    m_memory.arm7.iwram.fill(0);
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
    m_rom->Read(&m_memory.ewram[0x3FFE00], 0, sizeof(Header));

    const auto LoadBinary = [&](Header::Binary& binary, bool arm9) {
      const char* name = arm9 ? "ARM9" : "ARM7";

      const u32 size = binary.size;

      const u32 file_address_lo = binary.file_address;
      const u32 file_address_hi = file_address_lo + size;

      const u32 load_address_lo = binary.load_address;
      const u32 load_address_hi = load_address_lo + size;

      bool bad_header;

      bad_header  = file_address_hi > m_rom->Size();
      bad_header |= file_address_hi < file_address_lo;
      bad_header |= load_address_hi < load_address_lo;

      if(bad_header) {
        ATOM_PANIC("bad NDS file header (bad {} binary descriptor)", name);
      }

      bool loaded = false;

      if(load_address_lo >= 0x02000000u && load_address_hi < 0x023FFFFFu) {
        m_rom->Read(&m_memory.ewram[load_address_lo & 0x3FFFFF], file_address_lo, size);
        loaded = true;
      }

      if(load_address_lo >= 0x03800000u && load_address_hi < 0x03810000u) {
        m_rom->Read(&m_memory.arm7.iwram[load_address_lo & 0xFFFF], file_address_lo, size);
        loaded = true;
      }

      if(!loaded) {
        ATOM_PANIC("failed to load {} binary into memory: load_address=0x{:08X} size={}", name, load_address_lo, size);
      }

    };

    LoadBinary(header.arm9, true);
    LoadBinary(header.arm7, false);

    /**
     * This is required for direct booting commercial ROMs.
     * Thank you Hydr8gon for pointing it out to me.
     * @todo: do not write addresses that are not required.
     */
    atom::write<u32>(m_memory.ewram.data(), 0x3FF800, 0x1FC2); // Chip ID 1
    atom::write<u32>(m_memory.ewram.data(), 0x3FF804, 0x1FC2); // Chip ID 2
    atom::write<u16>(m_memory.ewram.data(), 0x3FF850, 0x5835); // ARM7 BIOS CRC
    atom::write<u16>(m_memory.ewram.data(), 0x3FF880, 0x0007); // Message from ARM9 to ARM7
    atom::write<u16>(m_memory.ewram.data(), 0x3FF884, 0x0006); // ARM7 boot task
    atom::write<u32>(m_memory.ewram.data(), 0x3FFC00, 0x1FC2); // Copy of chip ID 1
    atom::write<u32>(m_memory.ewram.data(), 0x3FFC04, 0x1FC2); // Copy of chip ID 2
    atom::write<u16>(m_memory.ewram.data(), 0x3FFC10, 0x5835); // Copy of ARM7 BIOS CRC
    atom::write<u16>(m_memory.ewram.data(), 0x3FFC40, 0x0001); // Boot indicator

    // @todo: set ARM9 and ARM7 POSTFLG registers to one
  }

} // namespace dual::nds
