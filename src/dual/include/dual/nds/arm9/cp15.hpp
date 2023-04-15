
#pragma once

#include <atom/bit.hpp>
#include <dual/arm/cpu.hpp>
#include <dual/arm/memory.hpp>

namespace dual::nds::arm9 {

  class CP15 final : public arm::Coprocessor {
    public:
      CP15(arm::CPU* cpu, arm::Memory* bus);

      void Reset() override;
      void DirectBoot();
      u32  MRC(int opc1, int cn, int cm, int opc2) override;
      void MCR(int opc1, int cn, int cm, int opc2, u32 value) override;

    private:
      arm::CPU* m_cpu;
      arm::Memory* m_bus;

      union Control {
        atom::Bits< 0, 1, u32> enable_pu;
        atom::Bits< 2, 1, u32> enable_dcache;
        atom::Bits< 7, 1, u32> enable_big_endian_mode;
        atom::Bits<12, 1, u32> enable_icache;
        atom::Bits<13, 1, u32> alternate_vector_select;
        atom::Bits<14, 1, u32> enable_round_robin_replacement;
        atom::Bits<15, 1, u32> disable_loading_tbit;
        atom::Bits<16, 1, u32> enable_dtcm;
        atom::Bits<17, 1, u32> enable_dtcm_load_mode;
        atom::Bits<18, 1, u32> enable_itcm;
        atom::Bits<19, 1, u32> enable_itcm_load_mode;

        u32 word = 0;
      } m_control{};
  };

} // namespace dual::nds::arm9