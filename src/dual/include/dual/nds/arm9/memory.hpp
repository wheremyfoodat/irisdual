
#pragma once

#include <atom/logger/logger.hpp>
#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <atom/punning.hpp>
#include <dual/arm/memory.hpp>
#include <dual/nds/system_memory.hpp>

namespace dual::nds::arm9 {

  namespace bit = atom::bit;

  class MemoryBus final : public dual::arm::Memory {
    public:
      explicit MemoryBus(SystemMemory& memory) : m_ewram{memory.ewram.data()} {}

      u8  ReadByte(u32 address, Bus bus) override;
      u16 ReadHalf(u32 address, Bus bus) override;
      u32 ReadWord(u32 address, Bus bus) override;

      void WriteByte(u32 address, u8  value, Bus bus) override;
      void WriteHalf(u32 address, u16 value, Bus bus) override;
      void WriteWord(u32 address, u32 value, Bus bus) override;

    private:
      template<typename T> T    Read (u32 address, Bus bus);
      template<typename T> void Write(u32 address, T value, Bus bus);

      u8* m_ewram;
  };

} // namespace dual::nds::arm9
