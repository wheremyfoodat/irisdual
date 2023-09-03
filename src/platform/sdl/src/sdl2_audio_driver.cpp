
#include <atom/logger/logger.hpp>

#include "sdl2_audio_driver.hpp"

bool SDL2AudioDriver::Open(void* user_data, Callback callback, uint sample_rate, uint buffer_size) {
  SDL_AudioSpec want{};

  if(SDL_Init(SDL_INIT_AUDIO) < 0) {
    ATOM_ERROR("SDL_Init(SDL_INIT_AUDIO) failed.");
    return false;
  }

  want.freq = (int)sample_rate;
  want.samples = buffer_size;
  want.format = AUDIO_S16;
  want.channels = 2;
  want.callback = (SDL_AudioCallback)callback;
  want.userdata = user_data;

  m_audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &m_have, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

  if(m_audio_device == 0) {
    ATOM_ERROR("SDL_OpenAudioDevice: failed to open audio: %s\n", SDL_GetError());
    return false;
  }

  if(m_have.format != want.format) {
    ATOM_ERROR("SDL_AudioDevice: S16 sample format unavailable.");
    return false;
  }

  if(m_have.channels != want.channels) {
    ATOM_ERROR("SDL_AudioDevice: Stereo output unavailable.");
    return false;
  }

  SDL_PauseAudioDevice(m_audio_device, 0);
  return true;
}

void SDL2AudioDriver::Close() {
  SDL_CloseAudioDevice(m_audio_device);
}

uint SDL2AudioDriver::GetBufferSize() const {
  return m_have.samples;
}

void SDL2AudioDriver::QueueSamples(std::span<i16> buffer) {
  SDL_QueueAudio(m_audio_device, buffer.data(), buffer.size() * sizeof(i16));
}

uint SDL2AudioDriver::GetNumberOfQueuedSamples() const {
  return SDL_GetQueuedAudioSize(m_audio_device) / (2 * sizeof(i16));
}