
#include <atom/logger/logger.hpp>
#include <atom/panic.hpp>
#include <dual/nds/arm7/rtc.hpp>

#include <ctime>

namespace dual::nds {

  constexpr int RTC::s_argument_count[8];

  void RTC::Reset() {
    m_current_bit = 0;
    m_current_byte = 0;
    m_data = 0u;
    m_port = {};
    m_state = State::Command;

    m_stat1 = 0u;
    m_stat2 = 0u;
  }

  auto RTC::Read_RTC() -> u8 {
    return (m_port.sio << 0) |
           (m_port.sck << 1) |
           (m_port.cs  << 2) |
           (m_port.write_sio ? 16 : 0) |
           (m_port.write_sck ? 32 : 0) |
           (m_port.write_cs  ? 64 : 0);
  }

  void RTC::Write_RTC(u8 value) {
    const int old_sck = m_port.sck;
    const int old_cs  = m_port.cs;

    m_port.write_sio = value & 16;
    m_port.write_sck = value & 32;
    m_port.write_cs  = value & 64;

    if(m_port.write_sio) m_port.sio = (value >> 0) & 1;
    if(m_port.write_sck) m_port.sck = (value >> 1) & 1;
    if(m_port.write_cs ) m_port.cs  = (value >> 2) & 1;

    if(!old_cs && m_port.cs) {
      m_state = State::Command;
      m_current_bit  = 0;
      m_current_byte = 0;
    }

    if(!m_port.cs || !(old_sck && !m_port.sck)) {
      return;
    }

    switch(m_state) {
      case State::Command: {
        ReceiveCommandSIO();
        break;
      }
      case State::Receiving: {
        ReceiveBufferSIO();
        break;
      }
      case State::Sending: {
        TransmitBufferSIO();
        break;
      }
    }
  }

  bool RTC::ReadSIO() {
    m_data &= ~(1 << m_current_bit);
    m_data |= m_port.sio << m_current_bit;

    if(++m_current_bit == 8) {
      m_current_bit = 0;
      return true;
    }

    return false;
  }

  void RTC::ReceiveCommandSIO() {
    bool completed = ReadSIO();

    if(!completed) {
      return;
    }

    // Check whether the command should be interpreted MSB-first or LSB-first.
    if((m_data >> 4) == 6) {
      m_data = (m_data << 4) | (m_data >> 4);
      m_data = ((m_data & 0x33) << 2) | ((m_data & 0xCC) >> 2);
      m_data = ((m_data & 0x55) << 1) | ((m_data & 0xAA) >> 1);
    } else if((m_data & 15) != 6) {
      ATOM_ERROR("arm7: RTC: received command in unknown format, data=0x{0:X}", m_data);
      return;
    }

    m_reg = static_cast<Register>((m_data >> 4) & 7);
    m_current_bit  = 0;
    m_current_byte = 0;

    // data[7:] determines whether the RTC register will be read or written.
    if(m_data & 0x80) {
      ReadRegister();

      if(s_argument_count[(int)m_reg] > 0) {
        m_state = State::Sending;
      } else {
        m_state = State::Command;
      }
    } else {
      if(s_argument_count[(int)m_reg] > 0) {
        m_state = State::Receiving;
      } else {
        WriteRegister();
        m_state = State::Command;
      }
    }
  }

  void RTC::ReceiveBufferSIO() {
    if(m_current_byte < s_argument_count[(int)m_reg] && ReadSIO()) {
      m_buffer[m_current_byte] = m_data;

      if(++m_current_byte == s_argument_count[(int)m_reg]) {
        WriteRegister();

        // @todo: does the chip accept more commands or
        // must it be reenabled before sending the next command?
        m_state = State::Command;
      }
    }
  }

  void RTC::TransmitBufferSIO() {
    m_port.sio = m_buffer[m_current_byte] & 1;
    m_buffer[m_current_byte] >>= 1;

    if(++m_current_bit == 8) {
      m_current_bit = 0;
      if(++m_current_byte == s_argument_count[(int)m_reg]) {
        // @todo: does the chip accept more commands or
        // must it be reenabled before sending the next command?
        m_state = State::Command;
      }
    }
  }

  void RTC::ReadRegister() {
    switch(m_reg) {
      case Register::Stat1: {
        m_buffer[0] = m_stat1;
        break;
      }
      case Register::Stat2: {
        m_buffer[0] = m_stat2;
        break;
      }
      case Register::Time: {
        const auto timestamp = std::time(nullptr);
        const auto time = std::localtime(&timestamp);

        m_buffer[0] = ConvertDecimalToBCD(time->tm_hour);
        m_buffer[1] = ConvertDecimalToBCD(time->tm_min);
        m_buffer[2] = ConvertDecimalToBCD(time->tm_sec);
        break;
      }
      case Register::DateTime: {
        const auto timestamp = std::time(nullptr);
        const auto time = std::localtime(&timestamp);

        m_buffer[0] = ConvertDecimalToBCD(time->tm_year - 100);
        m_buffer[1] = ConvertDecimalToBCD(1 + time->tm_mon);
        m_buffer[2] = ConvertDecimalToBCD(time->tm_mday);
        m_buffer[3] = ConvertDecimalToBCD(time->tm_wday);
        m_buffer[4] = ConvertDecimalToBCD(time->tm_hour);
        m_buffer[5] = ConvertDecimalToBCD(time->tm_min);
        m_buffer[6] = ConvertDecimalToBCD(time->tm_sec);
        break;
      }
      case Register::INT1_01: {
        // @todo
        m_buffer[0] = 0;
        m_buffer[1] = 0;
        m_buffer[2] = 0;
        break;
      }
      case Register::INT2_AlarmTime: {
        // @todo
        m_buffer[0] = 0;
        m_buffer[1] = 0;
        m_buffer[2] = 0;
        break;
      }
      case Register::ClockAdjustment: {
        // @todo
        m_buffer[0] = 0;
        break;
      }
      default: {
        ATOM_PANIC("arm7: RTC: unhandled register read: {}", (int)m_reg);
      }
    }
  }

  void RTC::WriteRegister() {
    switch(m_reg) {
      case Register::Stat1: {
        m_stat1 &= m_stat1 & ~0b1110;
        m_stat1 |= m_buffer[0] & 0b1110;

        if(m_buffer[0] & 1) {
          // @todo: reset RTC
        }
        break;
      }
      case Register::Stat2: {
        // @todo: update what register 0x01 maps to.
        m_stat2 = m_buffer[0];
        break;
      }
      case Register::DateTime:
      case Register::Time: {
        // @todo
        ATOM_WARN("arm7: RTC: write to the (date)time register is unsupported.");
        break;
      }
      case Register::INT1_01: {
        // @todo
        break;
      }
      case Register::INT2_AlarmTime: {
        // @todo
        break;
      }
      case Register::ClockAdjustment: {
        // @todo
        break;
      }
      default: {
        ATOM_PANIC("arm7: RTC: unhandled register write: {}", (int)m_reg);
      }
    }
  }

} // namespace dual::nds
