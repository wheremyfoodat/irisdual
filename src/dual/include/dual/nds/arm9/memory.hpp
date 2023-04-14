
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

        switch(address >> 24) {
          case 0x02: {
            return atom::read<T>(m_ewram, address & 0x3FFFFFu);
          }
          case 0x04: {
            ATOM_ERROR("arm9: unhandled {}-bit IO read from 0x{:08X}", bit::number_of_bits<T>(), address);
            return 0;
          }
        }

        ATOM_PANIC("unhandled {}-bit read from 0x{:08X}", bit::number_of_bits<T>(), address);
      }

      template<typename T> void Write(u32 address, T value, Bus bus) {
        address &= ~(sizeof(T) - 1u);

        switch(address >> 24) {
          case 0x02: {
            atom::write<T>(m_ewram, address & 0x3FFFFFu, value);
            break;
          }
          case 0x04: {
            ATOM_ERROR("arm9: unhandled {}-bit IO write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
            break;
          }
          default: {
            ATOM_PANIC("unhandled {}-bit write to 0x{:08X} = 0x{:08X}", bit::number_of_bits<T>(), address, value);
          }
        }
      }

      u8* m_ewram;
  };

} // namespace dual::nds::arm9
