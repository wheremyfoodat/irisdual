
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/irq.hpp>

namespace dual::nds {

  class Timer {
    public:
      Timer(Scheduler& scheduler, IRQ& irq) : m_scheduler{scheduler}, m_irq{irq} {}

      void Reset();

      auto  Read_TMCNT(int id) -> u32;
      void Write_TMCNT(int id, u32 value, u32 mask);

    private:

      void ScheduleTimerOverflow(int id, int cycle_offset);
      void DoOverflow(int id);
      u16  GetTicksSinceLastReload(int id);

      Scheduler& m_scheduler;
      IRQ& m_irq;

      struct Channel {
        union TMCNT {
          atom::Bits< 0, 16, u32> reload;
          atom::Bits<16,  2, u32> clock_divider;
          atom::Bits<18,  1, u32> clock_select;
          atom::Bits<22,  1, u32> enable_irq;
          atom::Bits<23,  1, u32> enable;

          u32 word = 0u;
        } tmcnt{};

        u16 counter = 0u;
        int divider_shift{};
        u64 timestamp_last_reload{};
        Scheduler::Event* event = nullptr;
      } m_channel[4];
  };

} // namespace dual::nds
