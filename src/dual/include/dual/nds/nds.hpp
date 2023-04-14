
#pragma once

#include <dual/nds/arm9/memory.hpp>
#include <dual/nds/rom.hpp>
#include <dual/nds/system_memory.hpp>
#include <memory>

namespace dual::nds {

  class NDS {
    public:
      NDS();

      void Reset();
      void Step(int cycles);
      void LoadROM(std::shared_ptr<ROM> rom);
      void DirectBoot();

    private:
      SystemMemory m_memory{};

      struct ARM9 {
        arm9::MemoryBus bus;

        explicit ARM9(SystemMemory& memory) : bus{memory} {}
      } arm9{m_memory};

      std::shared_ptr<ROM> m_rom;
  };

} // namespace dual::nds
