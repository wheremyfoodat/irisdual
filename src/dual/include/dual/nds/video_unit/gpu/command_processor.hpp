
#pragma once

#include <atom/logger/logger.hpp>
#include <atom/integer.hpp>
#include <atom/panic.hpp>
#include <dual/common/fifo.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/video_unit/gpu/renderer/renderer_base.hpp>
#include <dual/nds/video_unit/gpu/geometry_engine.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>
#include <dual/nds/video_unit/gpu/registers.hpp>
#include <dual/nds/irq.hpp>

namespace dual::nds::gpu {

  class CommandProcessor {
    public:
      explicit CommandProcessor(
        Scheduler& scheduler,
        IRQ& arm9_irq,
        IO& io,
        GeometryEngine& geometry_engine
      );

      void Reset();

      const Matrix4<Fixed20x12>& GetClipMatrix() {
        if(m_clip_mtx_dirty) {
          m_clip_mtx = m_projection_mtx * m_coordinate_mtx;
          m_clip_mtx_dirty = false;
        }

        return m_clip_mtx;
      }

      const Matrix4<Fixed20x12>& GetVecMatrix() {
        return m_direction_mtx;
      }

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

      void SwapBuffers(gpu::RendererBase* renderer);

      [[nodiscard]] const Viewport& GetViewport() const {
        return m_viewport;
      }

    private:
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
      void cmdMatrixRestore();
      void cmdMatrixLoadIdentity();
      void cmdMatrixLoad4x4();
      void cmdMatrixLoad4x3();
      void cmdMatrixMultiply4x4();
      void cmdMatrixMultiply4x3();
      void cmdMatrixMultiply3x3();
      void cmdMatrixScale();
      void cmdMatrixTranslate();
      void cmdSetColor();
      void cmdSetNormal();
      void cmdSetUV();
      void cmdSubmitVertex16();
      void cmdSubmitVertex10();
      void cmdSubmitVertexXY();
      void cmdSubmitVertexXZ();
      void cmdSubmitVertexYZ();
      void cmdSubmitVertexDelta();
      void cmdSetPolygonAttributes();
      void cmdSetTextureParameters();
      void cmdSetPaletteBase();
      void cmdSetDiffuseAndAmbientMaterialColors();
      void cmdSetSpecularAndEmissiveMaterialColors();
      void cmdSetLightVector();
      void cmdSetLightColor();
      void cmdSetShininessTable();
      void cmdBeginVertices();
      void cmdEndVertices();
      void cmdSwapBuffers();
      void cmdViewport();

      Matrix4<Fixed20x12> DequeueMatrix4x4();
      Matrix4<Fixed20x12> DequeueMatrix4x3();
      Matrix4<Fixed20x12> DequeueMatrix3x3();
      void ApplyMatrixToCurrent(const Matrix4<Fixed20x12>& rhs_matrix);

      void SubmitVertex(const Vector3<Fixed20x12>& position) {
        m_last_position = position;
        m_geometry_engine.SubmitVertex(position, GetClipMatrix());
      }

      Scheduler& m_scheduler;
      IRQ& m_arm9_irq;
      GXSTAT& m_gxstat;
      GeometryEngine& m_geometry_engine;

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
      Matrix4<Fixed20x12> m_clip_mtx;

      Vector3<Fixed20x12> m_last_position;

      bool m_manual_translucent_y_sorting_pending{};
      bool m_swap_buffers_pending{};
      bool m_use_w_buffer_pending{};

      Viewport m_viewport{};
  };

} // namespace dual::nds::gpu
