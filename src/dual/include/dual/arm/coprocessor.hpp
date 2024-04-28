
#pragma once

#include <atom/integer.hpp>

namespace dual::arm {

  class CPU;

  class Coprocessor {
    public:
      virtual ~Coprocessor() = default;

      virtual void Reset() {}
      virtual void SetCPU(CPU* cpu) {}

      virtual u32  MRC(int opc1, int cn, int cm, int opc2) = 0;
      virtual void MCR(int opc1, int cn, int cm, int opc2, u32 value) = 0;
  };

} // namespace dual::arm
