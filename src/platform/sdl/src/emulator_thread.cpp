
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
    m_nds->Step(559241);
  }
}
