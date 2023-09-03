
#include <dual/audio_driver.hpp>

#include <SDL.h>

class SDL2AudioDriver final : public dual::AudioDriverBase {
  public:
    bool Open(void* user_data, Callback callback, uint sample_rate, uint buffer_size) override;
    void Close() override;
    uint GetBufferSize() const override;
    void QueueSamples(std::span<i16> buffer) override;
    uint GetNumberOfQueuedSamples() const override;

  private:
    SDL_AudioDeviceID m_audio_device{};
    SDL_AudioSpec m_have{};
};