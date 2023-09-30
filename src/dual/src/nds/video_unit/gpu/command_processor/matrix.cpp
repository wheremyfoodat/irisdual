
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdMatrixMode() {
    m_mtx_mode = (int)DequeueFIFO() & 3;
  }

  void CommandProcessor::cmdMatrixPush() {
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

  void CommandProcessor::cmdMatrixPop() {
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

  void CommandProcessor::cmdMatrixStore() {
    const u32 parameter = (u32)DequeueFIFO();

    switch(m_mtx_mode) {
      case 0: m_projection_mtx_stack = m_projection_mtx; break;
      case 1:
      case 2: {
        const int stack_address = (int)(parameter & 31u);

        if(stack_address == 31) {
          m_gxstat.matrix_stack_error_flag = 1;
        }
        m_coordinate_mtx_stack[stack_address] = m_coordinate_mtx;
        m_direction_mtx_stack[stack_address] = m_direction_mtx;
        break;
      }
      case 3: m_texture_mtx_stack = m_texture_mtx; break;
    }
  }

  void CommandProcessor::cmdMatrixIdentity() {
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

  void CommandProcessor::cmdMatrixLoad4x4() {
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

  void CommandProcessor::cmdMatrixLoad4x3() {
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

  void CommandProcessor::cmdMatrixMultiply4x4() {
    Matrix4<Fixed20x12> rhs_matrix;
    DequeueMatrix4x4(rhs_matrix);
    MultiplyCurrentMatrixWithMatrix(rhs_matrix);
  }

  void CommandProcessor::cmdMatrixMultiply4x3() {
    Matrix4<Fixed20x12> rhs_matrix;
    DequeueMatrix4x3(rhs_matrix);
    MultiplyCurrentMatrixWithMatrix(rhs_matrix);
  }

  void CommandProcessor::cmdMatrixMultiply3x3() {
    Matrix4<Fixed20x12> rhs_matrix;
    DequeueMatrix3x3(rhs_matrix);
    MultiplyCurrentMatrixWithMatrix(rhs_matrix);
  }

  void CommandProcessor::cmdMatrixScale() {
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

  void CommandProcessor::cmdMatrixTranslate() {
    Matrix4<Fixed20x12> rhs_matrix;

    rhs_matrix[0][0] = Fixed20x12::FromInt(1);
    rhs_matrix[1][1] = Fixed20x12::FromInt(1);
    rhs_matrix[2][2] = Fixed20x12::FromInt(1);
    rhs_matrix[3][0] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][1] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][2] = (i32)(u32)DequeueFIFO();
    rhs_matrix[3][3] = Fixed20x12::FromInt(1);

    MultiplyCurrentMatrixWithMatrix(rhs_matrix);
  }

  void CommandProcessor::DequeueMatrix4x4(Matrix4<Fixed20x12>& m) {
    for(int col = 0; col < 4; col++) {
      for(int row = 0; row < 4; row++) {
        m[col][row] = (i32)(u32)DequeueFIFO();
      }
    }
  }

  void CommandProcessor::DequeueMatrix4x3(Matrix4<Fixed20x12>& m) {
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

  void CommandProcessor::DequeueMatrix3x3(Matrix4<Fixed20x12>& m) {
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


  void CommandProcessor::MultiplyCurrentMatrixWithMatrix(const Matrix4<Fixed20x12>& rhs_matrix) {
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

} // namespace dual::nds::gpu
