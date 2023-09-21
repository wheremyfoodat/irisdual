
#pragma once

#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <dual/common/fifo.hpp>

namespace dual::nds::gpu {

  class CommandProcessor {
    public:
      void Reset() {
      }

      void Write_GXFIFO(u32 packed_cmds) {
        ATOM_PANIC("gpu: Unhandled write to GXFIFO: 0x{:08X}", packed_cmds);
      }

      void Write_GXFIFOCMD(u8 cmd, u32 value) {

      }

    private:
      FIFO<u64, 256> m_fifo;
  };

} // namespace dual::nds::gpu
