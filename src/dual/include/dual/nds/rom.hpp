
#pragma once

#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <cstring>

namespace dual::nds {

  class ROM {
    public:
      virtual ~ROM() = default;

      virtual size_t Size() const = 0;
      virtual void Read(u8* destination, u32 address, size_t size) const = 0;
  };

  class MemoryROM final : public ROM {
    public:
      MemoryROM(u8* data, size_t size) : m_data{data}, m_size{size} {}

     ~MemoryROM() override {
        delete m_data;
      }

      size_t Size() const override {
        return m_size;
      }

      void Read(u8* destination, u32 address, size_t size) const override {
        const u32 address_hi = address + size;

        if(address_hi > m_size || address_hi < address) {
          //ATOM_PANIC("out-of-bounds ROM read request: address=0x{:08X}, size={}", address, size);
          return;
        }
        std::memcpy(destination, &m_data[address], size);
      }

    private:
      u8* m_data;
      size_t m_size;
  };

} // namespace dual::nds
