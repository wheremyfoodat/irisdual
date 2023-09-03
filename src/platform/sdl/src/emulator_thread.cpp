
#include <atom/panic.hpp>

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
  // m_nds->GetVideoUnit().SetPresentationCallback(std::bind<EmulatorThread>(&EmulatorThread::PresentCallback, this));
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

void EmulatorThread::ThreadMain() {
  while(m_running) {
    m_frame_limiter.Run([this]() {
      m_nds->Step(559241);
    }, [](float){});
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
