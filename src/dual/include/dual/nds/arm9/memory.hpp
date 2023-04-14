
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <dual/arm/memory.hpp>
#include <dual/nds/system_memory.hpp>

namespace dual::nds::arm9 {

  class MemoryBus final : public dual::arm::Memory {
    public:
      explicit MemoryBus(SystemMemory& memory) : m_memory{memory} {}

      u8 ReadByte(u32 address, Bus bus) override {
        return Read<u8>(address, bus);
      }

      u16 ReadHalf(u32 address, Bus bus) override {
        return Read<u16>(address, bus);
      }

      u32 ReadWord(u32 address, Bus bus) override {
        return Read<u32>(address, bus);
      }

      void WriteByte(u32 address, u8 value, Bus bus) override {
        Write<u8>(address, value, bus);
      }

      void WriteHalf(u32 address, u16 value, Bus bus) override {
        Write<u16>(address, value, bus);
      }

      void WriteWord(u32 address, u32 value, Bus bus) override {
        Write<u32>(address, value, bus);
      }

    private:
      template<typename T> T Read(u32 address, Bus bus) {
        address &= ~(sizeof(T) - 1u);

        ATOM_PANIC("unhandled {}-bit read from 0x{:08X}", atom::bit::number_of_bits<T>(), address);
      }

      template<typename T> void Write(u32 address, T value, Bus bus) {
        address &= ~(sizeof(T) - 1u);

        ATOM_PANIC("unhandled {}-bit write to 0x{:08X} = 0x{:08X}", atom::bit::number_of_bits<T>(), address, value);
      }

      SystemMemory& m_memory;
  };

} // namespace dual::nds::arm9
