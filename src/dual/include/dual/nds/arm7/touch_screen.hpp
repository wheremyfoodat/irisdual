
#pragma once

#include <atom/integer.hpp>
#include <dual/nds/arm7/spi.hpp>

namespace dual::nds::arm7 {

  // Texas Instruments TSC2046 touch screen controller emulation
  class TouchScreen final : public SPI::Device {
    public:
      struct CalibrationData {
        int adc_x1;
        int adc_y1;
        int adc_x2;
        int adc_y2;
        int screen_x1;
        int screen_y1;
        int screen_x2;
        int screen_y2;
      };

      void Reset() override;
      void Select() override;
      void Deselect() override;

      u8 Transfer(u8 data) override;

      void SetCalibrationData(const CalibrationData& data);

    private:
      enum class Channel {
        TouchScreen_Y = 1,
        TouchScreen_X = 5
      };

      u16 m_data_out{};

      int m_adc_x_top_left{};
      int m_adc_x_delta{};
      int m_adc_y_top_left{};
      int m_adc_y_delta{};
      int m_screen_x_top_left{};
      int m_screen_x_delta{1};
      int m_screen_y_top_left{};
      int m_screen_y_delta{1};
  };

} // namespace dual::nds::arm7
