
#include <algorithm>
#include <atom/punning.hpp>
#include <dual/nds/nds.hpp>
#include <dual/nds/header.hpp>

#include "arm/arm.hpp"

namespace dual::nds {

  NDS::NDS() {
    m_arm9.cpu = std::make_unique<arm::ARM>(&m_arm9.bus, arm::CPU::Model::ARM9);
    m_arm9.cp15 = std::make_unique<arm9::CP15>(m_arm9.cpu.get(), &m_arm9.bus);
    m_arm9.cpu->SetCoprocessor(15, m_arm9.cp15.get());
    m_arm9.irq.SetCPU(m_arm9.cpu.get());

    m_arm7.cpu = std::make_unique<arm::ARM>(&m_arm7.bus, arm::CPU::Model::ARM7);
    m_arm7.irq.SetCPU(m_arm7.cpu.get());
  }

  void NDS::Reset() {
    m_scheduler.Reset();

    m_video_unit.Reset();

    m_memory.ewram.fill(0);
    m_memory.swram.Reset();
    m_memory.vram.Reset();
    m_memory.pram.fill(0);
    m_memory.oam.fill(0);
    m_memory.arm9.dtcm.fill(0);
    m_memory.arm9.itcm.fill(0);
    m_memory.arm7.iwram.fill(0);

    m_arm9.cp15->Reset();
    m_arm9.cpu->Reset();
    m_arm7.cpu->Reset();

    m_arm9.irq.Reset();
    m_arm7.irq.Reset();

    m_arm9.dma.Reset();
    m_arm7.dma.Reset();

    m_arm7.spi.Reset();

    m_ipc.Reset();

    m_step_target = 0u;
  }

  void NDS::Step(int cycles_to_run) {
    const u64 step_target = m_step_target + cycles_to_run;

    while(m_scheduler.GetTimestampNow() < step_target) {
      // @todo: do not sync so tightly when either CPU is halted.
      const u64 target = std::min(m_scheduler.GetTimestampTarget(), step_target);
      const int cycles = std::min(32, static_cast<int>(target - m_scheduler.GetTimestampNow()));

      m_arm9.cpu->Run(cycles * 2);
      m_arm7.cpu->Run(cycles);

      m_scheduler.AddCycles(cycles);
    }

    m_step_target = step_target;
  }

  void NDS::LoadBootROM9(std::span<u8, 0x8000> data) {
    std::copy(data.begin(), data.end(), m_memory.arm9.bios.begin());
  }

  void NDS::LoadBootROM7(std::span<u8, 0x4000> data) {
    std::copy(data.begin(), data.end(), m_memory.arm7.bios.begin());
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
      bad_header |= (size & 3u) != 0u;

      if(bad_header) {
        ATOM_PANIC("bad NDS file header (bad {} binary descriptor)", name);
      }

      if(arm9) {
        for(u32 i = 0; i < size; i += 4u) {
          u32 word;

          m_rom->Read((u8*)&word, file_address_lo + i, sizeof(u32));
          m_arm9.bus.WriteWord(load_address_lo + i, word, dual::arm::Memory::Bus::System);
        }
      } else {
        for(u32 i = 0; i < size; i += 4u) {
          u32 word;

          m_rom->Read((u8*)&word, file_address_lo + i, sizeof(u32));
          m_arm7.bus.WriteWord(load_address_lo + i, word, dual::arm::Memory::Bus::System);
        }
      }
    };

    LoadBinary(header.arm9, true);
    LoadBinary(header.arm7, false);

    using Mode = arm::CPU::Mode;

    m_arm9.cpu->SetGPR(arm::CPU::GPR::SP, Mode::System,     0x03002F7Cu);
    m_arm9.cpu->SetGPR(arm::CPU::GPR::SP, Mode::IRQ,        0x03003F80u);
    m_arm9.cpu->SetGPR(arm::CPU::GPR::SP, Mode::Supervisor, 0x03003FC0u);
    m_arm9.cpu->SetGPR(arm::CPU::GPR::PC, header.arm9.entrypoint);
    m_arm9.cpu->SetCPSR(static_cast<u32>(Mode::System));

    m_arm9.cp15->DirectBoot();

    m_arm7.cpu->SetGPR(arm::CPU::GPR::SP, Mode::System,     0x0380FD80u);
    m_arm7.cpu->SetGPR(arm::CPU::GPR::SP, Mode::IRQ,        0x0380FF80u);
    m_arm7.cpu->SetGPR(arm::CPU::GPR::SP, Mode::Supervisor, 0x0380FFC0u);
    m_arm7.cpu->SetGPR(arm::CPU::GPR::PC, header.arm7.entrypoint);
    m_arm7.cpu->SetCPSR(static_cast<u32>(Mode::System));

    /**
     * This is required for direct booting commercial ROMs.
     * Thank you Hydr8gon for pointing it out to me.
     * @todo: do not write addresses that are not required.
     */
    atom::write<u32>(m_memory.ewram.data(), 0x3FF800u, 0x1FC2u); // Chip ID 1
    atom::write<u32>(m_memory.ewram.data(), 0x3FF804u, 0x1FC2u); // Chip ID 2
    atom::write<u16>(m_memory.ewram.data(), 0x3FF850u, 0x5835u); // ARM7 BIOS CRC
    atom::write<u16>(m_memory.ewram.data(), 0x3FF880u, 0x0007u); // Message from ARM9 to ARM7
    atom::write<u16>(m_memory.ewram.data(), 0x3FF884u, 0x0006u); // ARM7 boot task
    atom::write<u32>(m_memory.ewram.data(), 0x3FFC00u, 0x1FC2u); // Copy of chip ID 1
    atom::write<u32>(m_memory.ewram.data(), 0x3FFC04u, 0x1FC2u); // Copy of chip ID 2
    atom::write<u16>(m_memory.ewram.data(), 0x3FFC10u, 0x5835u); // Copy of ARM7 BIOS CRC
    atom::write<u16>(m_memory.ewram.data(), 0x3FFC40u, 0x0001u); // Boot indicator

    // @todo: set ARM9 and ARM7 POSTFLG registers to one
  }

} // namespace dual::nds
