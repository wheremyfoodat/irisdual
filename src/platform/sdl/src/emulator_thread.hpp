
#pragma once

#include <atomic>
#include <dual/nds/nds.hpp>
#include <optional>
#include <thread>
#include <mutex>
#include <queue>

class EmulatorThread {
  public:
    EmulatorThread();
   ~EmulatorThread();

    [[nodiscard]] bool IsRunning() const {
      return m_running;
    }

    void Start(std::unique_ptr<dual::nds::NDS> nds);
    std::unique_ptr<dual::nds::NDS> Stop();

    void SetTouchState(bool pen_down, u8 x, u8 y);

    [[nodiscard]] bool GetFastForward() const;
    void SetFastForward(bool fast_forward);

    std::optional<std::pair<const u32*, const u32*>> AcquireFrame();
    void ReleaseFrame();

  private:
    enum class MessageType : u8 {
      SetTouchState
    };

    struct Message {
      MessageType type;
      union {
        struct {
          u8 pen_down;
          u8 x;
          u8 y;
        } set_touch_state;
      };
    };

    void PushMessage(const Message& message);
    void ProcessMessages();

    void ThreadMain();
    void PresentCallback(const u32* fb_top, const u32* fb_bottom);

    std::unique_ptr<dual::nds::NDS> m_nds{};
    std::thread m_thread{};
    std::atomic_bool m_running{};
    std::atomic_bool m_fast_forward{};

    // Thread-safe UI thread to emulator thread message queue
    std::queue<Message> m_msg_queue{};
    std::mutex m_msg_queue_mutex{};

    struct FrameMailbox {
      u32 frames[2][2][256 * 192];
      int read_id = 0;
      int write_id = 1;
      std::atomic_bool available[2];
    } m_frame_mailbox;
};