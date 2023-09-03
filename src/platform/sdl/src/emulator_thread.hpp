
#pragma once

#include <atomic>
#include <dual/nds/nds.hpp>
#include <optional>
#include <thread>

#include "frame_limiter.hpp"

class EmulatorThread {
  public:
    EmulatorThread();
   ~EmulatorThread();

    [[nodiscard]] bool IsRunning() const {
      return m_running;
    }

    void Start(std::unique_ptr<dual::nds::NDS> nds);
    std::unique_ptr<dual::nds::NDS> Stop();

    std::optional<std::pair<const u32*, const u32*>> AcquireFrame();

  private:
    void ThreadMain();
    void PresentCallback(const u32* fb_top, const u32* fb_bottom);

    std::unique_ptr<dual::nds::NDS> m_nds{};
    std::thread m_thread{};
    std::atomic_bool m_running{};

    u32 m_frame_mailbox[2][256 * 192]{};
    std::atomic_bool m_frame_available{};

    // @todo: remove this once we have implemented audio sync
    nba::FrameLimiter m_frame_limiter{};
};