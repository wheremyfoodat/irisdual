
#include <atom/panic.hpp>
#include <chrono>

#include "emulator_thread.hpp"

EmulatorThread::EmulatorThread() = default;

EmulatorThread::~EmulatorThread() {
  Stop();
}

void EmulatorThread::Start(std::unique_ptr<dual::nds::NDS> nds) {
  if(m_running) {
    ATOM_PANIC("Starting an already running emulator thread is illegal.");
  }
  m_nds = std::move(nds);
  m_frame_available = false;
  m_nds->GetVideoUnit().SetPresentationCallback([this](const u32* fb_top, const u32* fb_bottom) {
    PresentCallback(fb_top, fb_bottom);
  });
  m_running = true;
  m_thread = std::thread{&EmulatorThread::ThreadMain, this};
}

std::unique_ptr<dual::nds::NDS> EmulatorThread::Stop() {
  if(!m_running) {
    return {};
  }
  m_running = false;
  m_thread.join();
  return std::move(m_nds);
}

bool EmulatorThread::GetFastForward() const {
  return m_fast_forward;
}

void EmulatorThread::SetFastForward(bool fast_forward) {
  if(fast_forward != m_fast_forward) {
    m_fast_forward = fast_forward;
    m_nds->GetAPU().SetEnableOutput(!fast_forward);
  }
}

void EmulatorThread::ThreadMain() {
  using namespace std::chrono_literals;

  constexpr int k_cycles_per_frame = 560190;

  dual::AudioDriverBase* audio_driver = m_nds->GetAPU().GetAudioDriver();

  if(!audio_driver) {
    ATOM_PANIC("An audio driver is required to synchronize to audio, but no audio driver is present.");
  }

  // We aim to keep at least four and at most eight audio buffers in the queue.
  const uint full_buffer_size = audio_driver->GetBufferSize() * 8;
  const uint half_buffer_size = full_buffer_size >> 1;

  while(m_running) {
    if(!m_fast_forward) {
      uint current_buffer_size = audio_driver->GetNumberOfQueuedSamples();

      // Sleep until the queue is less than half full
      while(current_buffer_size > half_buffer_size) {
        std::this_thread::sleep_for(1ms);

        current_buffer_size = audio_driver->GetNumberOfQueuedSamples();
      }

      // Run the emulator for as many cycles as is needed to fully fill the queue.
      const int cycles = (int)(full_buffer_size - current_buffer_size) * 1024;
      m_nds->Step(cycles);
    } else {
      m_nds->Step(k_cycles_per_frame);
    }
  }
}

std::optional<std::pair<const u32*, const u32*>> EmulatorThread::AcquireFrame() {
  // @todo: tearing can still happen, if a new frame becomes available during presentation.
  if(m_frame_available) {
    m_frame_available = false;
    return std::make_pair<const u32*, const u32*>(m_frame_mailbox[0], m_frame_mailbox[1]);
  }

  return std::nullopt;
}

void EmulatorThread::PresentCallback(const u32* fb_top, const u32* fb_bottom) {
  std::memcpy(m_frame_mailbox[0], fb_top, sizeof(u32) * 256 * 192);
  std::memcpy(m_frame_mailbox[1], fb_bottom, sizeof(u32) * 256 * 192);
  m_frame_available = true;
}
