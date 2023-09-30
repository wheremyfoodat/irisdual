
#pragma once

#include <atom/logger/logger.hpp>
#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <dual/common/fifo.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>
#include <dual/nds/video_unit/gpu/registers.hpp>
#include <dual/nds/irq.hpp>

namespace dual::nds::gpu {

  class CommandProcessor {
    public:
      explicit CommandProcessor(
        Scheduler& scheduler,
        IRQ& arm9_irq,
        GXSTAT& gxstat
      )   : m_scheduler{scheduler}
          , m_arm9_irq{arm9_irq}
          , m_gxstat{gxstat} {
      }

      void Reset() {
        m_unpack = {};
        m_cmd_pipe.Reset();
        m_cmd_fifo.Reset();
        m_mtx_mode = 0;
      }

      void Write_GXFIFO(u32 word) {
        if(m_unpack.params_left > 0) {
          EnqueueFIFO((u8)m_unpack.word, word);

          if(--m_unpack.params_left == 0) {
            m_unpack.word >>= 8;
            m_unpack.cmds_left--;
          }
          return;
        }

        if(m_unpack.cmds_left == 0) {
          m_unpack.cmds_left = 4;
          m_unpack.word = word;
        }

        for(int i = 0; i < 4; i++) {
          const u8 command = (u8)m_unpack.word;

          m_unpack.params_left = k_cmd_num_params[command];

          if(m_unpack.params_left != 0) {
            break;
          }

          EnqueueFIFO(command, 0u);
          m_unpack.word >>= 8;
          m_unpack.cmds_left--;

          if(m_unpack.cmds_left == 0 || m_unpack.word == 0u) {
            m_unpack.cmds_left = 0;
            break;
          }
        }
      }

      void Write_GXCMDPORT(u32 address, u32 param) {
        EnqueueFIFO((address & 0x1FFu) >> 2, param);
      }

      void UpdateIRQ() {
        m_arm9_irq.SetRequestGXFIFOFlag(ShouldRequestIRQ());
      }

    private:
      static constexpr int k_cmd_num_params[256] {
        0, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x00 - 0x0F (all NOPs)
        1, 0, 1, 1,  1, 0, 16, 12, 16, 12, 9, 3, 3, 0, 0, 0, // 0x10 - 0x1F (Matrix engine)
        1, 1, 1, 2,  1, 1,  1,  1,  1,  1, 1, 1, 0, 0, 0, 0, // 0x20 - 0x2F (Vertex and polygon attributes, mostly)
        1, 1, 1, 1, 32, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x30 - 0x3F (Material / lighting properties)
        1, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x40 - 0x4F (Begin/end vertex)
        1, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x50 - 0x5F (Swap buffers)
        1, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x60 - 0x6F (Set viewport)
        3, 2, 1                                              // 0x70 - 0x7F (Box, position and vector test)
      };

      void EnqueueFIFO(u8 command, u32 param) {
        const u64 entry = (u64)command << 32 | param;

        if(m_cmd_fifo.IsEmpty() && !m_cmd_pipe.IsFull()) {
          m_cmd_pipe.Write(entry);
        } else {
          if(m_cmd_fifo.IsFull()) {
            ATOM_PANIC("gpu: Attempted to write to full GXFIFO, busy={}", m_gxstat.busy);
          }

          m_cmd_fifo.Write(entry);
          UpdateFIFOStatus();
        }

        if(!m_gxstat.busy) {
          ProcessCommands();
        }
      }

      u64  DequeueFIFO() {
        if(m_cmd_pipe.IsEmpty()) {
          ATOM_PANIC("gpu: bad dequeue from empty GXPIPE");
        }

        const u64 entry = m_cmd_pipe.Read();

        if(m_cmd_pipe.Count() <= 2) {
          for(int i = 0; i < 2; i++) {
            if(m_cmd_fifo.IsEmpty()) break;

            m_cmd_pipe.Write(m_cmd_fifo.Read());
          }

          UpdateFIFOStatus();
        }

        return entry;
      }

      void UpdateFIFOStatus() {
        const auto fifo_size  = m_cmd_fifo.Count();

        m_gxstat.cmd_fifo_size = fifo_size;
        m_gxstat.cmd_fifo_empty = m_cmd_fifo.IsEmpty();
        m_gxstat.cmd_fifo_less_than_half_full = fifo_size < 128;

        UpdateIRQ();
      }

      [[nodiscard]] bool ShouldRequestIRQ() const {
        switch((GXSTAT::IRQ)m_gxstat.cmd_fifo_irq) {
          case GXSTAT::IRQ::Empty: return m_gxstat.cmd_fifo_empty;
          case GXSTAT::IRQ::LessThanHalfFull: return m_gxstat.cmd_fifo_less_than_half_full;
          default: return false;
        }
      }

      void ProcessCommands() {
        if(m_cmd_pipe.IsEmpty()) {
          m_gxstat.busy = false;
          return;
        }

        const u8 command = (u8)(m_cmd_pipe.Peek() >> 32);
        const size_t number_of_entries = m_cmd_pipe.Count() + m_cmd_fifo.Count();

        if(number_of_entries < k_cmd_num_params[command]) {
          m_gxstat.busy = false;
          return;
        }

        ExecuteCommand(command);

        m_gxstat.busy = true;
        // @todo: think of a more efficient solution.
        m_scheduler.Add(1, [this](int _) {
          ProcessCommands();
        });
      }

      void ExecuteCommand(u8 command) {
        switch(command) {
          case 0x00: DequeueFIFO(); break; // NOP
          case 0x10: cmdMtxMode(); break;
          case 0x11: cmdMtxPush(); break;
          case 0x12: cmdMtxPop(); break;
          case 0x15: cmdMtxIdentity(); break;
          case 0x16: cmdMtxLoad4x4(); break;
          case 0x17: cmdMtxLoad4x3(); break;
          case 0x18: cmdMtxMult4x4(); break;
          case 0x19: cmdMtxMult4x3(); break;
          case 0x1A: cmdMtxMult3x3(); break;
          case 0x1C: cmdMtxTrans(); break;
          case 0x40: cmdBeginVtxs(); break;
          case 0x41: cmdEndVtxs(); break;
          case 0x50: cmdSwapBuffers(); break;
          case 0x60: cmdViewport(); break;
          default: {
            if(k_cmd_num_params[command] == 0) {
              DequeueFIFO();
            }

            for(int i = 0; i < k_cmd_num_params[command]; i++) {
              DequeueFIFO();
            }

            if(
              command != 0x34 && /* SHININESS */
              command != 0x29 && /* POLYGON_ATTR */
              command != 0x2A && /* TEXIMAGE_PARAM */
              command != 0x2B && /* PLTT_BASE */
              true
            ) {
              ATOM_PANIC("gpu: Unimplemented command 0x{:02X}", command);
            }
          }
        }
      }

      void DequeueMatrix4x4(Matrix4<Fixed20x12>& m) {
        for(int col = 0; col < 4; col++) {
          for(int row = 0; row < 4; row++) {
            m[col][row] = (i32)(u32)DequeueFIFO();
          }
        }
      }

      void DequeueMatrix4x3(Matrix4<Fixed20x12>& m) {
        for(int col = 0; col < 4; col++) {
          for(int row = 0; row < 3; row++) {
            m[col][row] = (i32)(u32)DequeueFIFO();
          }
        }
        m[0][3] = 0;
        m[1][3] = 0;
        m[2][3] = 0;
        m[3][3] = Fixed20x12::FromInt(1);
      }

      void DequeueMatrix3x3(Matrix4<Fixed20x12>& m) {
        for(int col = 0; col < 3; col++) {
          for(int row = 0; row < 3; row++) {
            m[col][row] = (i32)(u32)DequeueFIFO();
          }
          m[col][3] = 0;
        }
        m[3][0] = 0;
        m[3][1] = 0;
        m[3][2] = 0;
        m[3][3] = Fixed20x12::FromInt(1);
      }

      void cmdMtxMode();
      void cmdMtxPush();
      void cmdMtxPop();
      void cmdMtxIdentity();
      void cmdMtxLoad4x4();
      void cmdMtxLoad4x3();
      void cmdMtxMult4x4();
      void cmdMtxMult4x3();
      void cmdMtxMult3x3();
      void cmdMtxTrans();
      void cmdBeginVtxs();
      void cmdEndVtxs();
      void cmdSwapBuffers();
      void cmdViewport();

      Scheduler& m_scheduler;
      IRQ& m_arm9_irq;
      GXSTAT& m_gxstat;

      struct Unpack {
        u32 word = 0u;
        int cmds_left = 0u;
        int params_left = 0u;
      } m_unpack;

      FIFO<u64, 4>   m_cmd_pipe;
      FIFO<u64, 256> m_cmd_fifo;

      // Matrix Engine
      int m_mtx_mode{};
      Matrix4<Fixed20x12> m_projection_mtx_stack;
      Matrix4<Fixed20x12> m_coordinate_mtx_stack[32];
      Matrix4<Fixed20x12> m_direction_mtx_stack[32];
      Matrix4<Fixed20x12> m_texture_mtx_stack;
      Matrix4<Fixed20x12> m_projection_mtx;
      Matrix4<Fixed20x12> m_coordinate_mtx;
      Matrix4<Fixed20x12> m_direction_mtx;
      Matrix4<Fixed20x12> m_texture_mtx;
      size_t m_projection_mtx_index;
      size_t m_coordinate_mtx_index;
      size_t m_texture_mtx_index;
  };

} // namespace dual::nds::gpu
