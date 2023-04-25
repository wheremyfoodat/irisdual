
#pragma once

#include <dual/arm/cpu.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/arm7/dma.hpp>
#include <dual/nds/arm7/memory.hpp>
#include <dual/nds/arm7/spi.hpp>
#include <dual/nds/arm9/cp15.hpp>
#include <dual/nds/arm9/math.hpp>
#include <dual/nds/arm9/memory.hpp>
#include <dual/nds/video_unit/video_unit.hpp>
#include <dual/nds/arm9/dma.hpp>
#include <dual/nds/cartridge.hpp>
#include <dual/nds/ipc.hpp>
#include <dual/nds/irq.hpp>
#include <dual/nds/rom.hpp>
#include <dual/nds/system_memory.hpp>
#include <dual/nds/timer.hpp>
#include <memory>
#include <span>

namespace dual::nds {

  class NDS {
    public:
      NDS();

      void Reset();
      void Step(int cycles_to_run);
      void LoadBootROM9(std::span<u8, 0x8000> data);
      void LoadBootROM7(std::span<u8, 0x4000> data);
      void LoadROM(std::shared_ptr<ROM> rom);
      void DirectBoot();

      VideoUnit& GetVideoUnit() {
        return m_video_unit;
      }

    private:
      Scheduler m_scheduler{};

      SystemMemory m_memory{};

      VideoUnit m_video_unit{m_scheduler, m_memory, m_arm9.irq, m_arm7.irq, m_arm9.dma, m_arm7.dma};

      Cartridge m_cartridge{m_scheduler, m_arm9.irq, m_arm7.irq, m_arm9.dma, m_arm7.dma, m_memory};

      struct ARM9 {
        std::unique_ptr<arm::CPU> cpu{};
        std::unique_ptr<arm9::CP15> cp15{};
        arm9::MemoryBus bus;
        IRQ irq{true};
        Timer timer;
        arm9::DMA dma{bus, irq};
        arm9::Math math{};

        ARM9(Scheduler& scheduler, SystemMemory& memory, IPC& ipc, VideoUnit& video_unit, Cartridge& cartridge)
            : bus{memory, {
                irq,
                timer,
                dma,
                ipc,
                memory.swram,
                memory.vram,
                math,
                video_unit,
                cartridge
              }}
            , timer{scheduler, irq} {}
      } m_arm9{m_scheduler, m_memory, m_ipc, m_video_unit, m_cartridge};

      struct ARM7 {
        std::unique_ptr<arm::CPU> cpu{};
        arm7::MemoryBus bus;
        IRQ irq{false};
        Timer timer;
        arm7::DMA dma{bus, irq};
        arm7::SPI spi{irq};

        ARM7(Scheduler& scheduler, SystemMemory& memory, IPC& ipc, VideoUnit& video_unit)
            : bus{memory, {
                irq,
                timer,
                dma,
                spi,
                ipc,
                memory.swram,
                memory.vram,
                video_unit
              }}
            , timer{scheduler, irq} {}
      } m_arm7{m_scheduler, m_memory, m_ipc, m_video_unit};

      IPC m_ipc{m_arm9.irq, m_arm7.irq};

      std::shared_ptr<ROM> m_rom;

      u64 m_step_target{};
  };

} // namespace dual::nds
