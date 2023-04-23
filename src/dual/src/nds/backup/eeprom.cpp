
#include <atom/panic.hpp>
#include <dual/nds/backup/eeprom.hpp>

namespace dual::nds {

  EEPROM::EEPROM(std::string const& save_path, Size size_hint, bool fram)
      : m_save_path(save_path)
      , m_size_hint(size_hint)
      , m_fram(fram) {
    Reset();
  }

  void EEPROM::Reset() {
    static const std::vector<size_t> k_backup_sizes {
      8192, 32768, 65536, 131072
    };

    size_t bytes = k_backup_sizes[(int)m_size_hint];

    m_file = BackupFile::OpenOrCreate(m_save_path, k_backup_sizes, bytes);
    m_mask = bytes - 1U;
    Deselect();

    switch(bytes) {
      case 8192: {
        m_size = Size::_8K;
        m_page_mask = 31;
        m_address_upper_half = 0x1000;
        m_address_upper_quarter = 0x1800;
        break;
      }
      case 32768: {
        // There is no known use of EEPROM with this size (only FRAM).
        m_size = Size::_32K;
        m_page_mask = 63;
        m_address_upper_half = 0x4000;
        m_address_upper_quarter = 0x6000;
        break;
      }
      case 65536: {
        m_size = Size::_64K;
        m_page_mask = 127;
        m_address_upper_half = 0x8000;
        m_address_upper_quarter = 0xC000;
        break;
      }
      case 131072: {
        m_size = Size::_128K;
        m_page_mask = 255;
        m_address_upper_half = 0x10000;
        m_address_upper_quarter = 0x18000;
        break;
      }
      default: {
        ATOM_UNREACHABLE();
      }
    }

    // FRAM consists of a single page spanning the full address space.
    if(m_fram) {
      m_page_mask = m_mask;
    }

    m_write_enable_latch = false;
    m_write_protect_mode = 0;
    m_status_reg_write_disable = false;
  }

  void EEPROM::Select() {
    if(m_state == State::Deselected) {
      m_state = State::ReceiveCommand;
    }
  }

  void EEPROM::Deselect() {
    if(m_current_cmd == Command::Write ||
      m_current_cmd == Command::WriteStatus) {
      m_write_enable_latch = false;
    }

    m_state = State::Deselected;
  }

  auto EEPROM::Transfer(u8 data) -> u8 {
    switch(m_state) {
      case State::ReceiveCommand: {
        ParseCommand(static_cast<Command>(data));
        break;
      }
      case State::ReadStatus: {
        return (m_write_enable_latch ? 2 : 0) |
               (m_write_protect_mode << 2) |
               (m_status_reg_write_disable ? 128 : 0);
      }
      case State::WriteStatus: {
        // @todo: figure out how to apply the SRWD bit.
        m_write_enable_latch = data & 2;
        m_write_protect_mode = (data >> 2) & 3;
        m_status_reg_write_disable = data & 128;
        m_state = State::Idle;
        break;
      }
      case State::ReadAddress0: {
        m_address = data << 16;
        m_state = State::ReadAddress1;
        break;
      }
      case State::ReadAddress1: {
        m_address |= data << 8;
        m_state = State::ReadAddress2;
        break;
      }
      case State::ReadAddress2: {
        m_address |= data;
        if(m_current_cmd == Command::Read) {
          m_state = State::Read;
        } else {
          m_state = State::Write;
        }
        break;
      }
      case State::Read: {
        return m_file->Read(m_address++ & m_mask);
      }
      case State::Write: {
        switch(m_write_protect_mode) {
          case 1: if (m_address >= m_address_upper_quarter) return 0xFF;
          case 2: if (m_address >= m_address_upper_half) return 0xFF;
          case 3: return 0xFF;
        }

        m_file->Write(m_address & m_mask, data);
        m_address = (m_address & ~m_page_mask) | ((m_address + 1) & m_page_mask);
        break;
      }
      case State::Idle: {
        break;
      }
      default: {
        ATOM_UNREACHABLE();
      }
    }

    return 0xFF;
  }

  void EEPROM::ParseCommand(Command cmd) {
    m_current_cmd = cmd;

    switch(cmd) {
      case Command::WriteEnable: {
        m_write_enable_latch = true;
        m_state = State::Idle;
        break;
      }
      case Command::WriteDisable: {
        m_write_enable_latch = false;
        m_state = State::Idle;
        break;
      }
      case Command::ReadStatus: {
        m_state = State::ReadStatus;
        break;
      }
      case Command::WriteStatus: {
        if(m_write_enable_latch) {
          m_state = State::WriteStatus;
        } else {
          m_state = State::Idle;
        }
        break;
      }
      case Command::Read: {
        m_address = 0;
        if(m_size == Size::_128K) {
          m_state = State::ReadAddress0;
        } else {
          m_state = State::ReadAddress1;
        }
        break;
      }
      case Command::Write: {
        if(m_write_enable_latch) {
          m_address = 0;
          if(m_size == Size::_128K) {
            m_state = State::ReadAddress0;
          } else {
            m_state = State::ReadAddress1;
          }
        } else {
          m_state = State::Idle;
        }
        break;
      }
      default: {
        ATOM_PANIC("EEPROM: unhandled command 0x{0:02X}", (int)cmd);
      }
    }
  }

} // namespace dual::nds
