
#include <atom/logger/logger.hpp>
#include <dual/nds/arm7/spi.hpp>
#include <dual/nds/arm7/touch_screen.hpp>
#include <dual/nds/backup/flash.hpp>

namespace dual::nds::arm7 {

  SPI::SPI(IRQ& irq) : m_irq{irq} {
    // @todo: better handle the case where firmware.bin does not exist.
    m_device_table[1] = std::make_unique<FLASH>("firmware.bin", FLASH::Size::_256K);
    m_device_table[2] = std::make_unique<TouchScreen>();
    ReadAndApplyTouchScreenCalibrationData();
  }

  void SPI::Reset() {
    m_spicnt = {};
    m_spidata = 0u;
    m_chip_select = false;

    for(auto& device : m_device_table) {
      if(device) {
        device->Reset();
      }
    }
  }

  auto SPI::Read_SPICNT() -> u16 {
    return m_spicnt.half;
  }

  void SPI::Write_SPICNT(u16 value, u16 mask) {
    const u16 write_mask = 0xFFFFu & mask;

    const uint old_device = m_spicnt.device;
    const bool old_enable = m_spicnt.enable;

    m_spicnt.half = (value & write_mask) | (m_spicnt.half & ~write_mask);

    if(m_spicnt.enable_16bit_mode) {
      ATOM_PANIC("arm7: SPI: enabled the bugged 16-bit mode");
    }

    const uint new_device = m_spicnt.device;
    const bool new_enable = m_spicnt.enable;

    if(m_chip_select && ((old_enable && !new_enable) || old_device != new_device)) {
      m_device_table[old_device]->Deselect();
      m_chip_select = false;
    }
  }

  auto SPI::Read_SPIDATA() -> u8 {
    return m_spidata;
  }

  void SPI::Write_SPIDATA(u8 value) {
    if(!m_spicnt.enable) {
      ATOM_ERROR("arm7: SPI: attempted to write SPIDATA while the SPI interface is disabled");
      return;
    }

    Device* device = m_device_table[m_spicnt.device].get();

    if(device) {
      if(!m_chip_select) {
        device->Select();
        m_chip_select = true;
      }

      m_spidata = device->Transfer(value);

      if(!m_spicnt.chip_select_hold) {
        device->Deselect();
        m_chip_select = false;
      }
    } else {
      ATOM_WARN("arm7: SPI: transfer byte from/to unimplemented device ({}): 0x{:02X}", m_spicnt.device, value);
    }

    if(m_spicnt.enable_irq) {
      m_irq.Raise(IRQ::Source::SPI);
    }
  }

  void SPI::ReadAndApplyTouchScreenCalibrationData() {
    auto& firmware = (FLASH&)*m_device_table[1];
    auto& touch_screen = (TouchScreen&)*m_device_table[2];

    const auto send_byte = [&](u8 byte) {
      firmware.Transfer(byte);
    };

    const auto send_address = [&](u32 address) {
      send_byte((u8)(address >> 16));
      send_byte((u8)(address >>  8));
      send_byte((u8)(address >>  0));
    };

    const auto read_byte = [&]() {
      return firmware.Transfer(0u);
    };

    const auto read_half = [&]() {
      u16 data = read_byte();
      return read_byte() << 8 | data;
    };

    TouchScreen::CalibrationData calibration_data{};

    u32 user_data_offset;
    firmware.Select();
    send_byte(0x03u);
    send_address(0x20);
    user_data_offset = read_half();
    firmware.Deselect();

    firmware.Select();
    send_byte(0x03u);
    send_address(user_data_offset * 8u + 0x58u);
    calibration_data.adc_x1 = read_half();
    calibration_data.adc_y1 = read_half();
    calibration_data.screen_x1 = read_byte();
    calibration_data.screen_y1 = read_byte();
    calibration_data.adc_x2 = read_half();
    calibration_data.adc_y2 = read_half();
    calibration_data.screen_x2 = read_byte();
    calibration_data.screen_y2 = read_byte();
    firmware.Deselect();

    touch_screen.SetCalibrationData(calibration_data);
  }

} // namespace dual::nds::arm7
