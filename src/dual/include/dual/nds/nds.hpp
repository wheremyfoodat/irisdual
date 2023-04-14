
#pragma once

#include <dual/nds/rom.hpp>
#include <memory>

namespace dual::nds {

  class NDS {
    public:
      NDS();

      void Reset();
      void Step(int cycles);
      void LoadROM(std::shared_ptr<ROM> rom);

    private:
  };

} // namespace dual::nds
