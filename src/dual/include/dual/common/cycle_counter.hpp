
#pragma once

#include <atom/integer.hpp>

namespace dual {

  class CycleCounter {
    public:
      explicit CycleCounter(int device_clock_rate_shift) : m_device_clock_rate_shift{device_clock_rate_shift} {
      }

      void Reset() {
        m_timestamp_dev = 0u;
        m_timestamp_sys = 0u;
      }

      u64 GetTimestampNow() const {
        return m_timestamp_sys;
      }

      void AddDeviceCycles(uint cycles) {
        m_timestamp_dev += cycles;
        m_timestamp_sys = m_timestamp_dev >> m_device_clock_rate_shift;
      }

    private:
      u64 m_timestamp_dev{};
      u64 m_timestamp_sys{};
      int m_device_clock_rate_shift;
  };

} // namespace dual
