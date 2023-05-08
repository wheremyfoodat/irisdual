
#pragma once

#include <atom/integer.hpp>

namespace dual {

  template <typename T, size_t size>
  class FIFO {
    public:
      FIFO() {
        Reset();
      }

      void Reset() {
        m_rd_ptr = 0;
        m_wr_ptr = 0;
        m_count  = 0;

        for(auto& entry : m_data) entry = {};
      }

      size_t Count() const {
        return m_count;
      }

      bool IsEmpty() const {
        return m_count == 0;
      }

      bool IsFull() const {
        return m_count == size;
      }

      T Peek() const {
        return m_data[m_rd_ptr];
      }

      T Read() {
        auto value = m_data[m_rd_ptr];

        if(!IsEmpty()) {
          m_rd_ptr = (m_rd_ptr + 1) % size;
          m_count--;
        }

        return value;
      }

      void Write(const T& value) {
        if(IsFull()) {
          return;
        }

        m_data[m_wr_ptr] = value;
        m_wr_ptr = (m_wr_ptr + 1) % size;
        m_count++;
      }

    private:
      size_t m_rd_ptr{};
      size_t m_wr_ptr{};
      size_t m_count{};

      T m_data[size];
  };

} // namespace dual
