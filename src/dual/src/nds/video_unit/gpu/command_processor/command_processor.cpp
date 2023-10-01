
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  CommandProcessor::CommandProcessor(
    Scheduler& scheduler,
    IRQ& arm9_irq,
    gpu::IO& io
  )   : m_scheduler{scheduler}
      , m_arm9_irq{arm9_irq}
      , m_gxstat{io.gxstat} {
  }

  void CommandProcessor::Reset() {
    m_unpack = {};
    m_cmd_pipe.Reset();
    m_cmd_fifo.Reset();
    m_mtx_mode = 0;
    m_projection_mtx_index = 0;
    m_coordinate_mtx_index = 0;
    m_texture_mtx_index = 0;
    m_clip_mtx_dirty = false;
  }

  void CommandProcessor::EnqueueFIFO(u8 command, u32 param) {
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
    const auto fifo_size  = m_cmd_fifo.Count();

    m_gxstat.cmd_fifo_size = fifo_size;
    m_gxstat.cmd_fifo_empty = m_cmd_fifo.IsEmpty();
    m_gxstat.cmd_fifo_less_than_half_full = fifo_size < 128;

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

  void CommandProcessor::ExecuteCommand(u8 command) {
    switch(command) {
      case 0x00: DequeueFIFO(); break; // NOP
      case 0x10: cmdMatrixMode(); break;
      case 0x11: cmdMatrixPush(); break;
      case 0x12: cmdMatrixPop(); break;
      case 0x13: cmdMatrixStore(); break;
      case 0x15: cmdMatrixLoadIdentity(); break;
      case 0x16: cmdMatrixLoad4x4(); break;
      case 0x17: cmdMatrixLoad4x3(); break;
      case 0x18: cmdMatrixMultiply4x4(); break;
      case 0x19: cmdMatrixMultiply4x3(); break;
      case 0x1A: cmdMatrixMultiply3x3(); break;
      case 0x1B: cmdMatrixScale(); break;
      case 0x1C: cmdMatrixTranslate(); break;
      case 0x40: cmdBeginVertices(); break;
      case 0x41: cmdEndVertices(); break;
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
          command != 0x30 && // DIF_AMB
          command != 0x31 && // SPE_EMI
          command != 0x32 && // LIGHT_VECTOR
          command != 0x33 && // LIGHT_COLOR
          command != 0x34 && // SHININESS
          command != 0x29 && // POLYGON_ATTR
          command != 0x2A && // TEXIMAGE_PARAM
          command != 0x2B && // PLTT_BASE
          true
          ) {
          ATOM_PANIC("gpu: Unimplemented command 0x{:02X}", command);
        }
      }
    }
  }


  void CommandProcessor::cmdBeginVertices() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdEndVertices() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdSwapBuffers() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdViewport() {
    DequeueFIFO();

    // ...
  }

} // namespace dual::nds::gpu
