
#pragma once

#include <atom/integer.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>

namespace dual::nds::gpu {

  template<int bit_precision>
  class Interpolator {
    public:
      void Setup(u16 w0, u16 w1, i32 x, i32 x_min, i32 x_max) {
        constexpr u16 mask = bit_precision == 9 ? 0x7Eu : 0x7Fu;

        m_lerp_factor = CalculateLerpFactor(x, x_min, x_max);

        if(w0 == w1 && (w0 & mask) == 0u && (w1 & mask) == 0u) {
          m_perp_factor = m_lerp_factor;
        } else {
          m_perp_factor = CalculatePerpFactor(w0, w1, x, x_min, x_max);
        }
      }

      template<typename T>
      [[nodiscard]] T Lerp(T a, T b) const {
        const u32 inv_lerp_factor = (1 << bit_precision) - m_lerp_factor;

        return (a * inv_lerp_factor + b * m_lerp_factor) >> bit_precision;
      }

      template<typename T>
      [[nodiscard]] T Perp(T a, T b) const {
        const u32 inv_perp_factor = (1 << bit_precision) - m_perp_factor;

        return (a * inv_perp_factor + b * m_perp_factor) >> bit_precision;
      }

      template<typename Derived, typename T, uint n>
      void Perp(
        const atom::detail::Vector<Derived, T, n>& a,
        const atom::detail::Vector<Derived, T, n>& b,
        atom::detail::Vector<Derived, T, n>& out
      ) const {
        const u32 inv_perp_factor = (1 << bit_precision) - m_perp_factor;

        for(uint i = 0; i < n; i++) {
          out[i] = (a[i].Raw() * inv_perp_factor + b[i].Raw() * m_perp_factor) >> bit_precision;
        }
      }

    private:
      [[nodiscard]] u32 CalculateLerpFactor(i32 x, i32 x_min, i32 x_max) const {
        const u32 numer = (x - x_min) << bit_precision;
        const u32 denom = x_max - x_min;

        if(denom == 0u) {
          return 0u;
        }
        return numer / denom;
      }

      [[nodiscard]] u32 CalculatePerpFactor(u16 w0, u16 w1, i32 x, i32 x_min, i32 x_max) const {
        u16 w0_numer = w0;
        u16 w0_denom = w0;
        u16 w1_denom = w1;

        if constexpr(bit_precision == 9) {
          w0_numer >>= 1;
          w0_denom >>= 1;
          w1_denom >>= 1;

          if((w0 & 1u) == 1u && (w1 & 1u) == 0u) {
            w0_denom++;
          }
        }

        const u32 t0 = x - x_min;
        const u32 t1 = x_max - x;
        const u32 numer = (t0 << bit_precision) * w0_numer;
        const u32 denom = t1 * w1_denom + t0 * w0_denom;

        if(denom == 0u) {
          return 0u;
        }
        return numer / denom;
      }

      u32 m_lerp_factor{}; //< linear interpolation factor
      u32 m_perp_factor{}; //< perspective-correct interpolation factor
  };

} // namespace dual::nds::gpu
