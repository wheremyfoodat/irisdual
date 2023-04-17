
#include <dual/nds/arm9/math.hpp>
#include <limits>
#include <cmath>

namespace dual::nds::arm9 {

  void Math::Reset() {
    m_divcnt = {};
    m_div_numerator = 0u;
    m_div_denominator = 0u;
    m_div_result = 0u;
    m_div_remainder = 0u;

    m_sqrtcnt = {};
    m_sqrt_param = 0u;
    m_sqrt_result = 0u;
  }

  u32 Math::Read_DIVCNT() {
    return m_divcnt.word;
  }

  u64 Math::Read_DIV_NUMER() {
    return m_div_numerator;
  }

  u64 Math::Read_DIV_DENOM() {
    return m_div_denominator;
  }

  u64 Math::Read_DIV_RESULT() {
    return m_div_result;
  }

  u64 Math::Read_DIVREM_RESULT() {
    return m_div_remainder;
  }

  void Math::Write_DIVCNT(u32 value, u32 mask) {
    const u32 write_mask = 3u & mask;

    m_divcnt.word = (value & write_mask) | (m_divcnt.word & ~write_mask);

    UpdateDivision();
  }

  void Math::Write_DIV_NUMER(u64 value, u64 mask) {
    m_div_numerator = (value & mask) | (m_div_numerator & ~mask);

    UpdateDivision();
  }

  void Math::Write_DIV_DENOM(u64 value, u64 mask) {
    m_div_denominator = (value & mask) | (m_div_denominator & ~mask);

    UpdateDivision();
  }

  u32 Math::Read_SQRTCNT() {
    return m_sqrtcnt.word;
  }

  u32 Math::Read_SQRT_RESULT() {
    return m_sqrt_result;
  }

  u64 Math::Read_SQRT_PARAM() {
    return m_sqrt_param;
  }

  void Math::Write_SQRTCNT(u32 value, u32 mask) {
    const u32 write_mask = 1u & mask;

    m_sqrtcnt.word = (value & write_mask) | (m_sqrtcnt.word & ~write_mask);

    UpdateSquareRoot();
  }

  void Math::Write_SQRT_PARAM(u64 value, u64 mask) {
    m_sqrt_param = (value & mask) | (m_sqrt_param & ~mask);

    UpdateSquareRoot();
  }

  void Math::UpdateDivision() {
    m_divcnt.error_divide_by_zero = m_div_denominator == 0u;

    switch((DivisionMode)m_divcnt.division_mode) {
      case DivisionMode::Reserved:
      case DivisionMode::S32_S32: {
        s32 numer = (s32)m_div_numerator;
        s32 denom = (s32)m_div_denominator;

        if(numer == std::numeric_limits<s32>::min() && denom == -1) {
          // @todo: what should the remainder value actually be?
          m_div_result = (u64)(u32)std::numeric_limits<s32>::min();
          m_div_remainder = 0u;
        } else if(denom == 0) {
          m_div_result = numer < 0 ? 0xFFFFFFFF00000001u : 0xFFFFFFFFu;
          m_div_remainder = (u64)numer;
        } else {
          m_div_result = (u64)(s64)(numer / denom);
          m_div_remainder = (u64)(s64)(numer % denom);
        }
        break;
      }
      case DivisionMode::S64_S32:
      case DivisionMode::S64_S64: {
        s64 numer = (s64)m_div_numerator;
        s64 denom = m_divcnt.division_mode == DivisionMode::S64_S64 ? (s64)m_div_denominator : (s64)(s32)m_div_denominator;

        if(numer == std::numeric_limits<s64>::min() && denom == -1) {
          // @todo: what should the remainder value actually be?
          m_div_result = (u64)std::numeric_limits<s64>::min();
          m_div_remainder = 0u;
        } else if(denom == 0) {
          m_div_result = numer < 0 ? 1u : 0xFFFFFFFFFFFFFFFFu;
          m_div_remainder = numer;
        } else {
          m_div_result = (u64)(numer / denom);
          m_div_remainder = (u64)(numer % denom);
        }
        break;
      }
    }
  }

  void Math::UpdateSquareRoot() {
    // @todo: is there an efficient solution which does not require floats?
    if(m_sqrtcnt.mode_64_bit) {
      m_sqrt_result = (u32)std::sqrt((long double)m_sqrt_param);
    } else {
      m_sqrt_result = (u32)std::sqrt((u32)m_sqrt_param);
    }
  }

} // namespace dual::nds::arm9
