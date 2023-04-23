
#include <atom/panic.hpp>
#include <dual/nds/backup/eeprom512b.hpp>

namespace dual::nds {

  EEPROM512B::EEPROM512B(std::string const& save_path)
      : m_save_path(save_path) {
    Reset();
  }

  void EEPROM512B::Reset() {
    static const std::vector<size_t> k_backup_sizes { 512 };

    size_t size = 512;
    m_file = BackupFile::OpenOrCreate(m_save_path, k_backup_sizes, size);
    Deselect();

    m_write_enable_latch = false;
    m_write_protect_mode = 0;
  }

  void EEPROM512B::Select() {
    if(m_state == State::Deselected) {
      m_state = State::ReceiveCommand;
    }
  }

  void EEPROM512B::Deselect() {
    if(m_current_cmd == Command::Write ||
       m_current_cmd == Command::WriteStatus) {
      m_write_enable_latch = false;
    }

    m_state = State::Deselected;
  }

  u8 EEPROM512B::Transfer(u8 data) {
    switch(m_state) {
      case State::ReceiveCommand: {
        ParseCommand(data);
        break;
      }
      case State::ReadStatus: {
        return (m_write_enable_latch ? 2 : 0) |
               (m_write_protect_mode << 2) | 0xF0;
      }
      case State::WriteStatus: {
        m_write_enable_latch = data & 2;
        m_write_protect_mode = (data >> 2) & 3;
        m_state = State::Idle;
        break;
      }
      case State::ReadAddress: {
        m_address |= data;
        if(m_current_cmd == Command::Read) {
          m_state = State::Read;
        } else {
          m_state = State::Write;
        }
        break;
      }
      case State::Read: {
        return m_file->Read(m_address++ & 0x1FF);
      }
      case State::Write: {
        switch(m_write_protect_mode) {
          case 1: if(m_address >= 0x180) return 0xFF;
          case 2: if(m_address >= 0x100) return 0xFF;
          case 3: return 0xFF;
        }

        m_file->Write(m_address, data);
        m_address = (m_address & ~15) | ((m_address + 1) & 15);
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

  void EEPROM512B::ParseCommand(u8 cmd) {
    auto a8 = cmd & 8;

    if((cmd & 0x80) == 0) cmd &= ~8;

    m_current_cmd = static_cast<Command>(cmd);

    switch(m_current_cmd) {
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
        m_address = a8 << 5;
        m_state = State::ReadAddress;
        break;
      }
      case Command::Write: {
        if(m_write_enable_latch) {
          m_address = a8 << 5;
          m_state = State::ReadAddress;
        } else {
          m_state = State::Idle;
        }
        break;
      }
      default: {
        // ATOM_PANIC("EEPROM: unhandled command 0x{0:02X}", cmd);
      }
    }
  }

} // namespace dual::nds
