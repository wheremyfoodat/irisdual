
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/enums.hpp>
#include <dual/nds/irq.hpp>

namespace dual::nds {

  class VideoUnit {
    public:

      VideoUnit(Scheduler& scheduler, IRQ& irq9, IRQ& irq7);

      void Reset();

      u16   Read_DISPSTAT(CPU cpu);
      void Write_DISPSTAT(CPU cpu, u16 value, u16 mask);

      u16 Read_VCOUNT();

    private:
      static constexpr int k_drawing_lines = 192;
      static constexpr int k_blanking_lines = 71;
      static constexpr int k_total_lines = k_drawing_lines + k_blanking_lines;

      void UpdateVerticalCounterMatchFlag(CPU cpu);

      void BeginHDraw(int late);
      void BeginHBlank(int late);

      Scheduler& m_scheduler;

      union DISPSTAT {
        atom::Bits<0, 1, u16> vblank_flag;
        atom::Bits<1, 1, u16> hblank_flag;
        atom::Bits<2, 1, u16> vmatch_flag;
        atom::Bits<3, 1, u16> enable_vblank_irq;
        atom::Bits<4, 1, u16> enable_hblank_irq;
        atom::Bits<5, 1, u16> enable_vmatch_irq;
        atom::Bits<7, 1, u16> vmatch_setting_msb;
        atom::Bits<8, 8, u16> vmatch_setting;

        u16 half = 0u;
      } m_dispstat[2];

      u16 m_vcount{};

      IRQ* m_irq[2]{};
  };

} // namespace dual::nds
