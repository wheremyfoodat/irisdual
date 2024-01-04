
#include <atom/logger/logger.hpp>
#include <dual/nds/arm7/touch_screen.hpp>

namespace dual::nds::arm7 {

  void TouchScreen::Reset() {
    m_data_out = 0u;
  }

  void TouchScreen::Select() {
  }

  void TouchScreen::Deselect() {
    m_data_out = 0u;
  }

  void TouchScreen::SetCalibrationData(const CalibrationData &data) {
    m_adc_x_top_left = data.adc_x1;
    m_adc_x_delta = data.adc_x2 - data.adc_x1;
    m_adc_y_top_left = data.adc_y1;
    m_adc_y_delta = data.adc_y2 - data.adc_y1;
    m_screen_x_top_left = data.screen_x1;
    m_screen_x_delta = data.screen_x2 - data.screen_x1;
    m_screen_y_top_left = data.screen_y1;
    m_screen_y_delta = data.screen_y2 - data.screen_y1;

    if(m_screen_x_delta == 0) m_screen_x_delta = 1;
    if(m_screen_y_delta == 0) m_screen_y_delta = 1;
  }

  void TouchScreen::SetTouchState(bool pen_down, u8 x, u8 y) {
    m_pen_down = pen_down;
    m_pen_x = x;
    m_pen_y = y;
  }

  u8 TouchScreen::Transfer(u8 data) {
    const u8 byte_out = (u8)(m_data_out >> 8);

    m_data_out <<= 8;

    u16 adc_x = 0x0000u;
    u16 adc_y = 0x0FFFu;

    if(m_pen_down) {
      adc_x = (u16)((m_pen_x - m_screen_x_top_left + 1) * m_adc_x_delta / m_screen_x_delta + m_adc_x_top_left);
      adc_y = (u16)((m_pen_y - m_screen_y_top_left + 1) * m_adc_y_delta / m_screen_y_delta + m_adc_y_top_left);
    }

    if(data & 0x80u) {
      const int channel = (data >> 4) & 7;

      switch((Channel)channel) {
        case Channel::TouchScreen_X: {
          m_data_out = adc_x << 3;
          break;
        }
        case Channel::TouchScreen_Y: {
          m_data_out = adc_y << 3;
          break;
        }
        default: {
          ATOM_WARN("arm7: TSC: read from unimplemented channel: {}", channel)

          m_data_out = 0x7FF8u;
          break;
        }
      }
    }

    return byte_out;
  }

} // namespace dual::nds::arm7