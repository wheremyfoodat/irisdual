
#pragma once

#include <atomic>
#include <dual/nds/nds.hpp>
#include <thread>

class EmulatorThread {
  public:
    EmulatorThread();
   ~EmulatorThread();

    [[nodiscard]] bool IsRunning() const {
      return m_running;
    }

    void Start(std::unique_ptr<dual::nds::NDS> nds);
    std::unique_ptr<dual::nds::NDS> Stop();

  private:
    void ThreadMain();

    std::unique_ptr<dual::nds::NDS> m_nds{};
    std::thread m_thread{};
    std::atomic_bool m_running{};
};