
#include <atom/panic.hpp>
#include <dual/nds/backup/flash.hpp>

/*
 * @todo:
 * - figure out the state after a command completed
 * - figure out what happens if /CS is not driven high after sending a command byte.
 * - generate a plausible manufacturer and device id instead of always using the same one.
 */

namespace dual::nds {

  FLASH::FLASH(const std::string& save_path, Size size_hint)
      : m_save_path(save_path)
      , m_size_hint(size_hint) {
    Reset();
  }

  void FLASH::Reset() {
    static const std::vector<size_t> k_backup_sizes {
      0x40000, 0x80000, 0x100000
    };

    m_file = BackupFile::OpenOrCreate(m_save_path, k_backup_sizes, k_backup_sizes[static_cast<int>(m_size_hint)]);
    m_mask = m_file->Size() - 1U;
    Deselect();

    m_write_enable_latch = false;
    m_deep_power_down = false;
  }

  void FLASH::Select() {
    if(m_state == State::Deselected) {
      m_state = State::ReceiveCommand;
    }
  }

  void FLASH::Deselect() {
    if(m_current_cmd == Command::PageWrite ||
       m_current_cmd == Command::PageProgram ||
       m_current_cmd == Command::PageErase ||
       m_current_cmd == Command::SectorErase) {
      m_write_enable_latch = false;
    }

    m_state = State::Deselected;
  }

  u8 FLASH::Transfer(u8 data) {
    switch(m_state) {
      case State::ReceiveCommand: {
        ParseCommand(static_cast<Command>(data));
        break;
      }
      case State::ReadJEDEC: {
        data = m_jedec_id[m_address];
        if(++m_address == 3) {
          m_state = State::Idle;
        }
        break;
      }
      case State::ReadStatus: {
        return m_write_enable_latch ? 2 : 0;
      }
      case State::SendAddress0: {
        m_address  = data << 16;
        m_state = State::SendAddress1;
        break;
      }
      case State::SendAddress1: {
        m_address |= data << 8;
        m_state = State::SendAddress2;
        break;
      }
      case State::SendAddress2: {
        m_address |= data;
        m_address &= m_mask;

        switch(m_current_cmd) {
          case Command::ReadData:     m_state = State::ReadData;    break;
          case Command::ReadDataFast: m_state = State::DummyByte;   break;
          case Command::PageWrite:    m_state = State::PageWrite;   break;
          case Command::PageProgram:  m_state = State::PageProgram; break;
          case Command::PageErase:    m_state = State::PageErase;   break;
          case Command::SectorErase:  m_state = State::SectorErase; break;
          default: ATOM_UNREACHABLE();
        }
        break;
      }
      case State::DummyByte: {
        m_state = State::ReadData;
        break;
      }
      case State::ReadData: {
        return m_file->Read(m_address++ & m_mask);
      }
      case State::PageWrite: {
        m_file->Write(m_address, data);
        m_address = (m_address & ~0xFF) | ((m_address + 1) & 0xFF);
        break;
      }
      case State::PageProgram: {
        // @todo: confirm that page program actually is a bitwise-AND operation.
        m_file->Write(m_address, m_file->Read(m_address) & data);
        m_address = (m_address & ~0xFF) | ((m_address + 1) & 0xFF);
        break;
      }
      case State::PageErase: {
        m_address &= ~0xFF;
        for(uint i = 0; i < 256; i++) {
          m_file->Write(m_address + i, 0xFF);
        }
        m_state = State::Idle;
        break;
      }
      case State::SectorErase: {
        m_address &= ~0xFFFF;
        for(uint i = 0; i < 0x10000; i++) {
          m_file->Write(m_address + i, 0xFF);
        }
        m_state = State::Idle;
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

  void FLASH::ParseCommand(Command cmd) {
    if(m_deep_power_down) {
      if(cmd == Command::ReleaseDeepPowerDown) {
        m_deep_power_down = false;
        m_state = State::Idle;
      }
      return;
    }

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
      case Command::ReadJEDEC: {
        m_address = 0;
        m_state = State::ReadJEDEC;
        break;
      }
      case Command::ReadStatus: {
        m_state = State::ReadStatus;
        break;
      }
      case Command::ReadData:
      case Command::ReadDataFast: {
        m_state = State::SendAddress0;
        break;
      }
      case Command::PageWrite:
      case Command::PageProgram:
      case Command::PageErase:
      case Command::SectorErase: {
        if(m_write_enable_latch) {
          m_state = State::SendAddress0;
        } else {
          m_state = State::Idle;
        }
        break;
      }
      case Command::DeepPowerDown: {
        m_deep_power_down = true;
        break;
      }
      default: {
        // ATOM_PANIC("FLASH: unhandled command 0x{0:02X}", cmd);
      }
    }
  }

} // namespace dual::nds