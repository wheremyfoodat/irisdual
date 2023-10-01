
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
        gpu::IO& io
      );

      void Reset();

      void Write_GXFIFO(u32 word) {
        if(m_unpack.params_left > 0) {
          EnqueuePackedCmdParam(word);
          return;
        }
        SubmitPackedCmdList(word);
      }

      void Write_GXCMDPORT(u32 address, u32 param) {
        EnqueueFIFO((address & 0x1FFu) >> 2, param);
      }

      [[nodiscard]] u32 Read_GXSTAT() const {
        m_gxstat.coordinate_stack_level = m_coordinate_mtx_index & 31;
        m_gxstat.projection_stack_level = m_projection_mtx_index;
        return m_gxstat.word;
      }

      void Write_GXSTAT(u32 value, u32 mask) {
        const u32 write_mask = mask & 0xC0000000u;

        if(value & mask & 0x8000u) {
          // @todo: confirm that this is the correct behavior.
          m_projection_mtx_index = 0;
          m_texture_mtx_index = 0;
          m_gxstat.matrix_stack_error_flag = 0;
        }

        m_gxstat.word = (m_gxstat.word & ~write_mask) | (value & write_mask);

        RequestOrClearIRQ();
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

      void EnqueueFIFO(u8 command, u32 param);
      u64  DequeueFIFO();
      void UpdateFIFOStatus();
      void RequestOrClearIRQ();
      [[nodiscard]] bool ShouldRequestIRQ() const;

      void SubmitPackedCmdList(u32 word);
      void EnqueuePackedCmdParam(u32 param);
      void UnpackNextCommands();

      void ProcessCommands();
      void ExecuteCommand(u8 command);

      void cmdMatrixMode();
      void cmdMatrixPush();
      void cmdMatrixPop();
      void cmdMatrixStore();
      void cmdMatrixLoadIdentity();
      void cmdMatrixLoad4x4();
      void cmdMatrixLoad4x3();
      void cmdMatrixMultiply4x4();
      void cmdMatrixMultiply4x3();
      void cmdMatrixMultiply3x3();
      void cmdMatrixScale();
      void cmdMatrixTranslate();
      void cmdBeginVertices();
      void cmdEndVertices();
      void cmdSwapBuffers();
      void cmdViewport();

      Matrix4<Fixed20x12> DequeueMatrix4x4();
      Matrix4<Fixed20x12> DequeueMatrix4x3();
      Matrix4<Fixed20x12> DequeueMatrix3x3();
      void ApplyMatrixToCurrent(const Matrix4<Fixed20x12>& rhs_matrix);

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
      size_t m_projection_mtx_index{};
      size_t m_coordinate_mtx_index{};
      size_t m_texture_mtx_index{};
      bool m_clip_mtx_dirty{};
  };

} // namespace dual::nds::gpu
