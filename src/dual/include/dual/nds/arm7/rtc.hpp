
#pragma once

#include <atom/integer.hpp>

namespace dual::nds {

  class RTC {
    public:
      enum class State {
        Command,
        Sending,
        Receiving,
        Complete
      };

      enum class Register {
        Stat1 = 0,
        INT1_01 = 1,
        DateTime = 2,
        ClockAdjustment = 3,
        Stat2 = 4,
        INT2_AlarmTime = 5,
        Time = 6
      };

      RTC() {
        Reset();
      }

      void Reset();

      auto  Read_RTC() -> u8;
      void Write_RTC(u8 value);

    private:
      bool ReadSIO();
      void ReceiveCommandSIO();
      void ReceiveBufferSIO();
      void TransmitBufferSIO();
      void ReadRegister();
      void WriteRegister();

      static auto ConvertDecimalToBCD(u8 x) -> u8 {
        u8 y = 0;
        u8 e = 1;

        while(x > 0) {
          y += (x % 10) * e;
          e *= 16;
          x /= 10;
        }

        return y;
      }

      int m_current_bit{};
      int m_current_byte{};

      Register m_reg{};
      u8 m_data{};
      u8 m_buffer[7]{};

      struct PortData {
        int sio = 0;
        int sck = 0;
        int cs = 0;

        bool write_sio = false;
        bool write_sck = false;
        bool write_cs = false;
      } m_port{};

      State m_state{};

      u8 m_stat1{};
      u8 m_stat2{};

      static constexpr int s_argument_count[8] = {
        1, // Stat1
        3, // INT1 frequency duty / alarm1 time
        7, // DateTime
        1, // ClockAdjustment
        1, // Stat2
        3, // INT2 alarm2 time
        3, // Time
        0, // @todo
      };
  };

} // namespace dual::nds
