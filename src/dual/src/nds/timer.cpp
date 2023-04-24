
#include <dual/nds/timer.hpp>

namespace dual::nds {

  void Timer::Reset() {
    for(auto& channel : m_channel) channel = {};
  }

  auto Timer::Read_TMCNT(int id) -> u32 {
    auto& channel = m_channel[id];

    u16 counter = channel.counter;

    if(channel.tmcnt.enable && channel.tmcnt.clock_select == 0u) {
      counter += GetTicksSinceLastReload(id);
    }

    return (channel.tmcnt.word & 0xFFFF0000u) | counter;
  }

  void Timer::Write_TMCNT(int id, u32 value, u32 mask) {
    static constexpr int k_divider_to_cycles[4] { 1, 64, 256, 1024 };
    static constexpr int k_divider_to_shift[4] { 0, 6, 8, 10 };

    const u32 write_mask = 0x00C7FFFFu & mask;

    auto& channel = m_channel[id];
    auto& tmcnt = channel.tmcnt;

    const bool old_enable = tmcnt.enable;

    if(channel.event != nullptr) {
      channel.counter += GetTicksSinceLastReload(id);
      m_scheduler.Cancel(channel.event);
      channel.event = nullptr;
    }

    tmcnt.word = (value & write_mask) | (tmcnt.word & ~write_mask);

    if(tmcnt.enable) {
      if(!old_enable) {
        channel.counter = tmcnt.reload;
      }

      channel.divider_cycles = k_divider_to_cycles[tmcnt.clock_divider];
      channel.divider_shift = k_divider_to_shift[tmcnt.clock_divider];

      if(tmcnt.clock_select == 0u) {
        ScheduleTimerOverflow(id, -(int)(m_scheduler.GetTimestampNow() & (channel.divider_cycles - 1)));
      }
    }
  }

  void Timer::ScheduleTimerOverflow(int id, int cycle_offset) {
    auto& channel = m_channel[id];

    const uint cycles = (0x10000u - channel.counter) << channel.divider_shift;

    // @todo: do not recreate the lambda every time the timer overflows
    channel.event = m_scheduler.Add(cycles + cycle_offset, [=, this](int cycles_late) {
      DoOverflow(id);
      ScheduleTimerOverflow(id, -cycles_late);
    });

    channel.timestamp_last_reload = m_scheduler.GetTimestampNow();
  }

  void Timer::DoOverflow(int id) {
    auto& channel = m_channel[id];

    channel.counter = channel.tmcnt.reload;

    if(channel.tmcnt.enable_irq) {
      m_irq.Raise((IRQ::Source)((u32)IRQ::Source::Timer0 << id));
    }

    if(id != 3) {
      auto& next_channel = m_channel[id + 1];

      if(next_channel.tmcnt.enable && next_channel.tmcnt.clock_select == 1u) {
        // @todo: optimize this?
        if(next_channel.counter == 0xFFFFu) {
          DoOverflow(id + 1);
        } else {
          next_channel.counter++;
        }
      }
    }
  }

  u16 Timer::GetTicksSinceLastReload(int id) {
    const auto& channel = m_channel[id];

    return (m_scheduler.GetTimestampNow() - channel.timestamp_last_reload) >> channel.divider_shift;
  }

} // namespace dual::nds
