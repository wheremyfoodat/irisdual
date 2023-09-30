
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdMtxMode() {
    m_mtx_mode = (int)DequeueFIFO() & 3;
  }

  void CommandProcessor::cmdMtxPush() {
    DequeueFIFO();

    switch(m_mtx_mode) {
      case 0: {
        if(m_projection_mtx_index > 0) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_projection_mtx_stack = m_projection_mtx;
        m_projection_mtx_index = (m_projection_mtx_index + 1) & 1;
        break;
      }
      case 1:
      case 2: {
        if(m_coordinate_mtx_index > 30) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_coordinate_mtx_stack[m_coordinate_mtx_index & 31] = m_coordinate_mtx;
        m_direction_mtx_stack[m_coordinate_mtx_index & 31] = m_direction_mtx;
        m_coordinate_mtx_index = (m_coordinate_mtx_index + 1) & 63;
        break;
      }
      case 3: {
        if(m_texture_mtx_index > 0) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_texture_mtx_stack = m_texture_mtx;
        m_texture_mtx_index = (m_texture_mtx_index + 1) & 1;
        break;
      }
    }
  }

  void CommandProcessor::cmdMtxPop() {
    const u32 parameter = (u32)DequeueFIFO();

    switch(m_mtx_mode) {
      case 0: {
        m_projection_mtx_index = (m_projection_mtx_index - 1) & 1;
        if(m_projection_mtx_index > 0) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_projection_mtx = m_projection_mtx_stack;
        break;
      }
      case 1:
      case 2: {
        const int stack_offset = (int)(parameter & 63u);

        m_coordinate_mtx_index = (m_coordinate_mtx_index - stack_offset) & 63;
        if(m_coordinate_mtx_index > 30) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_coordinate_mtx = m_coordinate_mtx_stack[m_coordinate_mtx_index & 31];
        m_direction_mtx = m_direction_mtx_stack[m_coordinate_mtx_index & 31];
        break;
      }
      case 3: {
        m_texture_mtx_index = (m_texture_mtx_index - 1) & 1;
        if(m_texture_mtx_index > 0) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_texture_mtx = m_texture_mtx_stack;
        break;
      }
    }
  }

  void CommandProcessor::cmdMtxStore() {
    const u32 parameter = (u32)DequeueFIFO();

    switch(m_mtx_mode) {
      case 0: m_projection_mtx_stack = m_projection_mtx; break;
      case 1:
      case 2: {
        const int stack_address = (int)(parameter & 31u);

        m_coordinate_mtx_stack[stack_address] = m_coordinate_mtx;
        m_direction_mtx_stack[stack_address] = m_direction_mtx;
        break;
      }
      case 3: m_texture_mtx_stack = m_texture_mtx; break;
    }
  }

  void CommandProcessor::cmdMtxIdentity() {
    DequeueFIFO();

    switch(m_mtx_mode) {
      case 0: m_projection_mtx = Matrix4<Fixed20x12>::Identity(); break;
      case 1: m_coordinate_mtx = Matrix4<Fixed20x12>::Identity(); break;
      case 2: {
        m_coordinate_mtx = Matrix4<Fixed20x12>::Identity();
        m_direction_mtx  = Matrix4<Fixed20x12>::Identity();
        break;
      }
      case 3: m_texture_mtx = Matrix4<Fixed20x12>::Identity(); break;
    }
  }

  void CommandProcessor::cmdMtxLoad4x4() {
    switch(m_mtx_mode) {
      case 0: DequeueMatrix4x4(m_projection_mtx); break;
      case 1: DequeueMatrix4x4(m_coordinate_mtx); break;
      case 2: {
        DequeueMatrix4x4(m_coordinate_mtx);
        m_direction_mtx = m_coordinate_mtx;
        break;
      }
      case 3: DequeueMatrix4x4(m_texture_mtx); break;
    }
  }

  void CommandProcessor::cmdMtxLoad4x3() {
    switch(m_mtx_mode) {
      case 0: DequeueMatrix4x3(m_projection_mtx); break;
      case 1: DequeueMatrix4x3(m_coordinate_mtx); break;
      case 2: {
        DequeueMatrix4x3(m_coordinate_mtx);
        m_direction_mtx = m_coordinate_mtx;
        break;
      }
      case 3: DequeueMatrix4x3(m_texture_mtx); break;
    }
  }

  void CommandProcessor::cmdMtxMult4x4() {
    Matrix4<Fixed20x12> rhs_matrix;

    DequeueMatrix4x4(rhs_matrix);

    switch(m_mtx_mode) {
      case 0: m_projection_mtx = m_projection_mtx * rhs_matrix; break;
      case 1: m_coordinate_mtx = m_coordinate_mtx * rhs_matrix; break;
      case 2: {
        m_coordinate_mtx = m_coordinate_mtx * rhs_matrix;
        m_direction_mtx = m_direction_mtx * rhs_matrix;
        break;
      }
      case 3: m_texture_mtx = m_texture_mtx * rhs_matrix; break;
    }
  }

  void CommandProcessor::cmdMtxMult4x3() {
    Matrix4<Fixed20x12> rhs_matrix;

    DequeueMatrix4x3(rhs_matrix);

    // @todo: the code below is the same as cmdMtxMult4x4().

    switch(m_mtx_mode) {
      case 0: m_projection_mtx = m_projection_mtx * rhs_matrix; break;
      case 1: m_coordinate_mtx = m_coordinate_mtx * rhs_matrix; break;
      case 2: {
        m_coordinate_mtx = m_coordinate_mtx * rhs_matrix;
        m_direction_mtx = m_direction_mtx * rhs_matrix;
        break;
      }
      case 3: m_texture_mtx = m_texture_mtx * rhs_matrix; break;
    }
  }

  void CommandProcessor::cmdMtxMult3x3() {
    Matrix4<Fixed20x12> rhs_matrix;

    DequeueMatrix3x3(rhs_matrix);

    // @todo: the code below is the same as cmdMtxMult4x4().

    switch(m_mtx_mode) {
      case 0: m_projection_mtx = m_projection_mtx * rhs_matrix; break;
      case 1: m_coordinate_mtx = m_coordinate_mtx * rhs_matrix; break;
      case 2: {
        m_coordinate_mtx = m_coordinate_mtx * rhs_matrix;
        m_direction_mtx = m_direction_mtx * rhs_matrix;
        break;
      }
      case 3: m_texture_mtx = m_texture_mtx * rhs_matrix; break;
    }
  }

  void CommandProcessor::cmdMtxScale() {
    Matrix4<Fixed20x12> rhs_matrix;

    rhs_matrix[0][0] = (i32)(u32)DequeueFIFO();
    rhs_matrix[1][1] = (i32)(u32)DequeueFIFO();
    rhs_matrix[2][2] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][3] = Fixed20x12::FromInt(1);

    switch(m_mtx_mode) {
      case 0: m_projection_mtx = m_projection_mtx * rhs_matrix; break;
      case 1:
      case 2: m_coordinate_mtx = m_coordinate_mtx * rhs_matrix; break;
      case 3: m_texture_mtx = m_texture_mtx * rhs_matrix; break;
    }
  }

  void CommandProcessor::cmdMtxTrans() {
    Matrix4<Fixed20x12> rhs_matrix;

    rhs_matrix[0][0] = Fixed20x12::FromInt(1);
    rhs_matrix[1][1] = Fixed20x12::FromInt(1);
    rhs_matrix[2][2] = Fixed20x12::FromInt(1);
    rhs_matrix[3][0] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][1] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][2] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][3] = Fixed20x12::FromInt(1);

    // @todo: the code below is the same as cmdMtxMult4x4().

    switch(m_mtx_mode) {
      case 0: m_projection_mtx = m_projection_mtx * rhs_matrix; break;
      case 1: m_coordinate_mtx = m_coordinate_mtx * rhs_matrix; break;
      case 2: {
        m_coordinate_mtx = m_coordinate_mtx * rhs_matrix;
        m_direction_mtx = m_direction_mtx * rhs_matrix;
        break;
      }
      case 3: m_texture_mtx = m_texture_mtx * rhs_matrix; break;
    }
  }

  void CommandProcessor::cmdBeginVtxs() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdEndVtxs() {
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
