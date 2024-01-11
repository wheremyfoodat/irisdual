
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

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

  CommandProcessor::CommandProcessor(
    Scheduler& scheduler,
    IRQ& arm9_irq,
    arm9::DMA& arm9_dma,
    IO& io,
    GeometryEngine& geometry_engine
  )   : m_scheduler{scheduler}
      , m_arm9_irq{arm9_irq}
      , m_arm9_dma{arm9_dma}
      , m_gxstat{io.gxstat}
      , m_geometry_engine{geometry_engine} {
  }

  void CommandProcessor::Reset() {
    m_unpack = {};
    m_cmd_pipe.Reset();
    m_cmd_fifo.Reset();
    m_cmd_event = nullptr;
    m_mtx_mode = 0;
    m_projection_mtx_index = 0;
    m_coordinate_mtx_index = 0;
    m_texture_mtx_index = 0;
    m_clip_mtx_dirty = false;
    m_clip_mtx = Matrix4<Fixed20x12>::Identity();
    m_last_position = {};
    m_manual_translucent_y_sorting_pending = false;
    m_swap_buffers_pending = false;
    m_use_w_buffer_pending = false;
  }

  void CommandProcessor::SwapBuffers(gpu::RendererBase* renderer) {
    if(m_swap_buffers_pending) {
      m_swap_buffers_pending = false;
      renderer->SetWBufferEnable(m_use_w_buffer_pending);
      m_geometry_engine.SetManualTranslucentYSorting(m_manual_translucent_y_sorting_pending);
      m_geometry_engine.SwapBuffers();
      ProcessCommands();
    }
  }

  void CommandProcessor::EnqueueFIFO(u8 command, u32 param) {
    const u64 entry = (u64)command << 32 | param;

    if(m_cmd_fifo.IsEmpty() && !m_cmd_pipe.IsFull()) {
      m_cmd_pipe.Write(entry);
    } else {
      /* HACK: before we drop any command or argument data,
       * execute the next command early.
       * In hardware enqueueing into full queue would stall the CPU or DMA,
       * but this is difficult to emulate accurately.
       *
       * A while-loop instead of an if-statement seems necessary here.
       * Probably because of GXFIFO DMAs triggered by processing commands.
       */
      while(m_cmd_fifo.IsFull()) {
        m_gxstat.busy = false;
        if(m_cmd_event) {
          m_scheduler.Cancel(m_cmd_event);
        }
        ProcessCommandsImpl();
      }

      m_cmd_fifo.Write(entry);
      UpdateFIFOStatus();
    }

    if(!m_gxstat.busy) {
      ProcessCommands();
    }
  }

  u64 CommandProcessor::DequeueFIFO() {
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

  void CommandProcessor::UpdateFIFOStatus() {
    const size_t fifo_size = m_cmd_fifo.Count();
    const bool   fifo_less_than_half_full = fifo_size < 128;

    m_gxstat.cmd_fifo_size  = fifo_size;
    m_gxstat.cmd_fifo_empty = m_cmd_fifo.IsEmpty();
    m_gxstat.cmd_fifo_less_than_half_full = fifo_less_than_half_full;

    m_arm9_dma.SetGXFIFOLessThanHalfFull(fifo_less_than_half_full);

    RequestOrClearIRQ();
  }

  void CommandProcessor::RequestOrClearIRQ() {
    m_arm9_irq.SetRequestGXFIFOFlag(ShouldRequestIRQ());
  }

  bool CommandProcessor::ShouldRequestIRQ() const {
    switch((GXSTAT::IRQ)m_gxstat.cmd_fifo_irq) {
      case GXSTAT::IRQ::Empty: return m_gxstat.cmd_fifo_empty;
      case GXSTAT::IRQ::LessThanHalfFull: return m_gxstat.cmd_fifo_less_than_half_full;
      default: return false;
    }
  }

  void CommandProcessor::SubmitPackedCmdList(u32 word) {
    if(m_unpack.cmds_left == 0) {
      m_unpack.cmds_left = 4;
      m_unpack.word = word;
      UnpackNextCommands();
    }
  }

  void CommandProcessor::EnqueuePackedCmdParam(u32 param) {
    const u8 command = (u8)m_unpack.word;

    EnqueueFIFO(command, param);

    if(--m_unpack.params_left == 0) {
      m_unpack.word >>= 8;

      if(--m_unpack.cmds_left > 0 && m_unpack.word != 0u) {
        UnpackNextCommands();
      } else {
        m_unpack.cmds_left = 0;
      }
    }
  }

  void CommandProcessor::UnpackNextCommands() {
    while(m_unpack.cmds_left > 0) {
      const u8 command = (u8)m_unpack.word;
      const int number_of_parameters = k_cmd_num_params[command];

      if(number_of_parameters == 0) {
        EnqueueFIFO(command, 0u);
        m_unpack.word >>= 8;
        m_unpack.cmds_left--;

        if(m_unpack.word == 0u) {
          m_unpack.cmds_left = 0;
        }
      } else {
        m_unpack.params_left = number_of_parameters;
        break;
      }
    }
  }

  void CommandProcessor::ProcessCommands() {
    if(m_cmd_pipe.IsEmpty() || m_swap_buffers_pending) {
      m_gxstat.busy = false;
      return;
    }
    ProcessCommandsImpl();
  }

  void CommandProcessor::ProcessCommandsImpl() {
    const u8 command = (u8)(m_cmd_pipe.Peek() >> 32);
    const size_t number_of_entries = m_cmd_pipe.Count() + m_cmd_fifo.Count();

    if(number_of_entries < k_cmd_num_params[command]) {
      m_gxstat.busy = false;
      return;
    }

    ExecuteCommand(command);

    if(!m_swap_buffers_pending) {
      // @todo: think of a more efficient solution.
      m_gxstat.busy = true;
      m_cmd_event = m_scheduler.Add(1, [this](int _) {
        m_cmd_event = nullptr;
        ProcessCommands();
      });
    }
  }

  void CommandProcessor::ExecuteCommand(u8 command) {
    switch(command) {
      case 0x00: DequeueFIFO(); break; // NOP
      case 0x10: cmdMatrixMode(); break;
      case 0x11: cmdMatrixPush(); break;
      case 0x12: cmdMatrixPop(); break;
      case 0x13: cmdMatrixStore(); break;
      case 0x14: cmdMatrixRestore(); break;
      case 0x15: cmdMatrixLoadIdentity(); break;
      case 0x16: cmdMatrixLoad4x4(); break;
      case 0x17: cmdMatrixLoad4x3(); break;
      case 0x18: cmdMatrixMultiply4x4(); break;
      case 0x19: cmdMatrixMultiply4x3(); break;
      case 0x1A: cmdMatrixMultiply3x3(); break;
      case 0x1B: cmdMatrixScale(); break;
      case 0x1C: cmdMatrixTranslate(); break;
      case 0x20: cmdSetColor(); break;
      case 0x21: cmdSetNormal(); break;
      case 0x22: cmdSetUV(); break;
      case 0x23: cmdSubmitVertex16(); break;
      case 0x24: cmdSubmitVertex10(); break;
      case 0x25: cmdSubmitVertexXY(); break;
      case 0x26: cmdSubmitVertexXZ(); break;
      case 0x27: cmdSubmitVertexYZ(); break;
      case 0x28: cmdSubmitVertexDelta(); break;
      case 0x29: cmdSetPolygonAttributes(); break;
      case 0x2A: cmdSetTextureParameters(); break;
      case 0x2B: cmdSetPaletteBase(); break;
      case 0x30: cmdSetDiffuseAndAmbientMaterialColors(); break;
      case 0x31: cmdSetSpecularAndEmissiveMaterialColors(); break;
      case 0x32: cmdSetLightVector(); break;
      case 0x33: cmdSetLightColor(); break;
      case 0x34: cmdSetShininessTable(); break;
      case 0x40: cmdBeginVertices(); break;
      case 0x41: cmdEndVertices(); break;
      case 0x50: cmdSwapBuffers(); break;
      case 0x60: cmdViewport(); break;
      case 0x70: cmdBoxTest(); break;
      default: {
        if(k_cmd_num_params[command] == 0) {
          DequeueFIFO();
        }

        for(int i = 0; i < k_cmd_num_params[command]; i++) {
          DequeueFIFO();
        }

        if(
          command != 0x70 && // BOX_TEST
          true
        ) {
          ATOM_PANIC("gpu: Unimplemented command 0x{:02X}", command);
        }
      }
    }
  }

  void CommandProcessor::cmdBeginVertices() {
    m_geometry_engine.Begin((u32)DequeueFIFO());
  }

  void CommandProcessor::cmdEndVertices() {
    DequeueFIFO();

    m_geometry_engine.End();
  }

  void CommandProcessor::cmdSwapBuffers() {
    const u32 parameter = DequeueFIFO();

    m_manual_translucent_y_sorting_pending = parameter & 1;
    m_use_w_buffer_pending = parameter & 2;
    m_swap_buffers_pending = true;
  }

  void CommandProcessor::cmdViewport() {
    const u32 parameter = DequeueFIFO();

    const int x0 = (int)(u8)(parameter >>  0);
    const int y0 = (int)(u8)(parameter >>  8);
    const int x1 = (int)(u8)(parameter >> 16);
    const int y1 = (int)(u8)(parameter >> 24);

    if(x0 > x1 || y0 > y1) {
      ATOM_PANIC("gpu: failed viewport validation: {} <= {}, {} <= {}\n", x0, x1, y0, y1);
    }

    m_viewport.x0 = x0;
    m_viewport.y0 = y0;
    m_viewport.width  = 1 + x1 - x0;
    m_viewport.height = 1 + y1 - y0;
  }

  void CommandProcessor::cmdBoxTest() {
    const Matrix4<Fixed20x12>& clip_matrix = GetClipMatrix();

    const u32 param0 = (u32)DequeueFIFO();
    const u32 param1 = (u32)DequeueFIFO();
    const u32 param2 = (u32)DequeueFIFO();

    const i16 x_min = (i16)(u16)(param0 >>  0);
    const i16 y_min = (i16)(u16)(param0 >> 16);
    const i16 z_min = (i16)(u16)(param1 >>  0);

    const i16 x_max = (i16)(x_min + (i16)(u16)(param1 >> 16));
    const i16 y_max = (i16)(y_min + (i16)(u16)(param2 >>  0));
    const i16 z_max = (i16)(z_min + (i16)(u16)(param2 >> 16));

    /**
     *      2--------6
     *   ╱ |     ╱ |
     * 1----|---5    |
     * |    3---|----7
     * | ╱     | ╱
     * 0--------4
     */
    const Vector4<Fixed20x12> v[8] {
      clip_matrix * Vector4<Fixed20x12>{x_min, y_min, z_min, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_min, y_max, z_min, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_min, y_max, z_max, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_min, y_min, z_max, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_max, y_min, z_min, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_max, y_max, z_min, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_max, y_max, z_max, Fixed20x12::FromInt(1)},
      clip_matrix * Vector4<Fixed20x12>{x_max, y_min, z_max, Fixed20x12::FromInt(1)}
    };

    m_gxstat.test_cmd_result =
      !m_geometry_engine.ClipPolygon({{ {v[0]}, {v[1]}, {v[2]}, {v[3]} }}, false).Empty() ||
      !m_geometry_engine.ClipPolygon({{ {v[4]}, {v[5]}, {v[6]}, {v[7]} }}, false).Empty() ||
      !m_geometry_engine.ClipPolygon({{ {v[1]}, {v[5]}, {v[4]}, {v[0]} }}, false).Empty() ||
      !m_geometry_engine.ClipPolygon({{ {v[2]}, {v[6]}, {v[7]}, {v[3]} }}, false).Empty() ||
      !m_geometry_engine.ClipPolygon({{ {v[1]}, {v[5]}, {v[6]}, {v[2]} }}, false).Empty() ||
      !m_geometry_engine.ClipPolygon({{ {v[0]}, {v[4]}, {v[7]}, {v[3]} }}, false).Empty();
  }

} // namespace dual::nds::gpu
