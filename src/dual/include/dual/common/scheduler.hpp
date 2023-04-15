
#pragma once

#include <atom/integer.hpp>
#include <functional>
#include <limits>

namespace dual {

  class Scheduler {
    public:
      Scheduler();
     ~Scheduler();

      template<class T>
      using EventMethod = void (T::*)(int);

      struct Event {
        std::function<void(int)> callback;
      private:
        friend class Scheduler;
        int handle;
        u64 timestamp;
      };

      u64 GetTimestampNow() const {
        return m_timestamp_now;
      }

      u64 GetTimestampTarget() const {
        if(m_heap_size == 0) {
          return std::numeric_limits<u64>::max();
        }
        return m_heap[0]->timestamp;
      }

      int GetRemainingCycleCount() const {
        return int(GetTimestampTarget() - GetTimestampNow());
      }

      void AddCycles(int cycles) {
        m_timestamp_now += cycles;
      }

      void Reset();
      void Step();
      auto Add(u64 delay, std::function<void(int)> callback) -> Event*;
      void Cancel(Event* event) { Remove(event->handle); }

      template<class T>
      auto Add(std::uint64_t delay, T* object, EventMethod<T> method) -> Event* {
        return Add(delay, [object, method](int cycles_late) {
          (object->*method)(cycles_late);
        });
      }

    private:
      static constexpr int k_event_limit = 64;

      static int Parent(int n) {
        return (n - 1) >> 1;
      }

      static int LeftChild(int n) {
        return n * 2 + 1;
      }

      static int RightChild(int n) {
        return n * 2 + 2;
      }

      void Remove(int n);
      void Swap(int i, int j);
      void Heapify(int n);

      u64 m_timestamp_now = 0u;
      int m_heap_size = 0;
      Event* m_heap[k_event_limit]{};
  };

} // namespace dual
