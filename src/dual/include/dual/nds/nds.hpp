
#pragma once

#include <dual/arm/cpu.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/arm7/memory.hpp>
#include <dual/nds/arm9/cp15.hpp>
#include <dual/nds/arm9/memory.hpp>
#include <dual/nds/rom.hpp>
#include <dual/nds/system_memory.hpp>
#include <memory>

namespace dual::nds {

  class NDS {
    public:
      NDS();

      void Reset();
      void Step(int cycles_to_run);
      void LoadROM(std::shared_ptr<ROM> rom);
      void DirectBoot();

      SystemMemory& GetSystemMemory() {
        return m_memory;
      }

    private:
      Scheduler m_scheduler{};

      SystemMemory m_memory{};

      struct ARM9 {
        std::unique_ptr<arm::CPU> cpu{};
        std::unique_ptr<arm9::CP15> cp15{};
        arm9::MemoryBus bus;

        explicit ARM9(SystemMemory& memory) : bus{memory} {}
      } m_arm9{m_memory};

      struct ARM7 {
        std::unique_ptr<arm::CPU> cpu{};
        arm7::MemoryBus bus;

        explicit ARM7(SystemMemory& memory) : bus{memory} {}
      } m_arm7{m_memory};

      std::shared_ptr<ROM> m_rom;

      u64 m_step_target{};
  };

} // namespace dual::nds
