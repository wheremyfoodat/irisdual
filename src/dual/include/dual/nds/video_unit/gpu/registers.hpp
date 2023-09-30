
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>

namespace dual::nds::gpu {

  struct GXSTAT {
    enum class IRQ {
      Never = 0,
      LessThanHalfFull = 1,
      Empty = 2,
      Reserved = 3
    };

    union {
      atom::Bits< 0, 1, u32> test_cmd_busy;
      atom::Bits< 1, 1, u32> test_cmd_result;
      atom::Bits< 8, 5, u32> model_view_stack_level;
      atom::Bits<13, 1, u32> projection_stack_level;
      atom::Bits<14, 1, u32> matrix_stack_busy;
      atom::Bits<15, 1, u32> matrix_stack_error_flag;
      atom::Bits<16, 9, u32> cmd_fifo_size;
      atom::Bits<25, 1, u32> cmd_fifo_less_than_half_full;
      atom::Bits<26, 1, u32> cmd_fifo_empty;
      atom::Bits<27, 1, u32> busy;
      atom::Bits<30, 2, u32> cmd_fifo_irq;

      u32 word = 0u;
    };
  };

  struct IO {
    GXSTAT gxstat;
  };

} // namespace dual::nds::gpu
