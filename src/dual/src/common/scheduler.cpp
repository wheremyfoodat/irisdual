
#include <atom/panic.hpp>
#include <dual/common/scheduler.hpp>

namespace dual {

  Scheduler::Scheduler() {
    for(int i = 0; i < k_event_limit; i++) {
      m_heap[i] = &m_pool[i];
      m_heap[i]->handle = i;
    }
    Reset();
  }

  void Scheduler::Reset() {
    m_heap_size = 0;
    m_timestamp_now = 0;
  }

  void Scheduler::Step() {
    const u64 now = GetTimestampNow();

    while(m_heap_size > 0 && m_heap[0]->timestamp <= now) {
      auto event = m_heap[0];

      event->callback(int(now - event->timestamp));

      // @note: the handle may have changed due to the event callback.
      Remove(event->handle);
    }
  }

  auto Scheduler::Add(u64 delay, std::function<void(int)> callback) -> Event* {
    int n = m_heap_size++;
    int p = Parent(n);

    if(m_heap_size > k_event_limit) {
      ATOM_PANIC("exceeded maximum number of scheduler events.");
    }

    auto event = m_heap[n];
    event->timestamp = GetTimestampNow() + delay;
    event->callback = callback;

    while(n != 0 && m_heap[p]->timestamp > m_heap[n]->timestamp) {
      Swap(n, p);
      n = p;
      p = Parent(n);
    }

    return event;
  }

  void Scheduler::Remove(int n) {
    Swap(n, --m_heap_size);

    int p = Parent(n);

    if(n != 0 && m_heap[p]->timestamp > m_heap[n]->timestamp) {
      do {
        Swap(n, p);
        n = p;
        p = Parent(n);
      } while (n != 0 && m_heap[p]->timestamp > m_heap[n]->timestamp);
    } else {
      Heapify(n);
    }
  }

  void Scheduler::Swap(int i, int j) {
    auto tmp = m_heap[i];
    m_heap[i] = m_heap[j];
    m_heap[j] = tmp;
    m_heap[i]->handle = i;
    m_heap[j]->handle = j;
  }

  void Scheduler::Heapify(int n) {
    const int l = LeftChild(n);
    const int r = RightChild(n);

    if(l < m_heap_size && m_heap[l]->timestamp < m_heap[n]->timestamp) {
      Swap(l, n);
      Heapify(l);
    }

    if(r < m_heap_size && m_heap[r]->timestamp < m_heap[n]->timestamp) {
      Swap(r, n);
      Heapify(r);
    }
  }

} // namespace dual