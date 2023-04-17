
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>

namespace dual::nds::arm9 {

  class Math {
    public:
      void Reset();

      u32 Read_DIVCNT();
      u64 Read_DIV_NUMER();
      u64 Read_DIV_DENOM();
      u64 Read_DIV_RESULT();
      u64 Read_DIVREM_RESULT();

      void Write_DIVCNT(u32 value, u32 mask);
      void Write_DIV_NUMER(u64 value, u64 mask);
      void Write_DIV_DENOM(u64 value, u64 mask);

      u32 Read_SQRTCNT();
      u32 Read_SQRT_RESULT();
      u64 Read_SQRT_PARAM();

      void Write_SQRTCNT(u32 value, u32 mask);
      void Write_SQRT_PARAM(u64 value, u64 mask);

    private:

      enum class DivisionMode : uint {
        S32_S32  = 0u,
        S64_S32  = 1u,
        S64_S64  = 2u,
        Reserved = 3u
      };

      void UpdateDivision();
      void UpdateSquareRoot();

      union DIVCNT {
        atom::Bits< 0, 2, u32> division_mode;
        atom::Bits<14, 1, u32> error_divide_by_zero;

        u32 word = 0u;
      } m_divcnt{};

      u64 m_div_numerator{};
      u64 m_div_denominator{};
      u64 m_div_result{};
      u64 m_div_remainder{};

      union SQRTCNT {
        atom::Bits<0, 1, u32> mode_64_bit;

        u32 word = 0u;
      } m_sqrtcnt{};

      u32 m_sqrt_result{};
      u64 m_sqrt_param{};
  };

} // namespace dual::nds::arm9
