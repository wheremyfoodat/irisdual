
#pragma once

#include <atom/integer.hpp>
#include <dual/nds/video_unit/gpu/math.hpp>

namespace dual::nds::gpu {

  template<int bit_precision>
  class Interpolator {
    public:
      void Setup(u16 w0, u16 w1, i32 x, i32 x_min, i32 x_max) {
        constexpr u16 mask = bit_precision == 9 ? 0x7Eu : 0x7Fu;
        m_use_lerp_fallback = w0 == w1 && (w0 & mask) == 0u && (w1 & mask) == 0u;

        const i32 t0 = x - x_min;
        const i32 t1 = x_max - x;

        if(x_min != x_max) {
          m_lerp_numer = t0;
          m_lerp_denom = x_max - x_min;

          if(!m_use_lerp_fallback) {
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

            const u32 numer = (u32)((t0 << bit_precision) * w0_numer);
            const u32 denom = (u32)(t1 * w1_denom + t0 * w0_denom);

            m_perp_factor = numer / denom;
          }
        } else {
          m_lerp_numer  = 0;
          m_lerp_denom  = 1;
          m_perp_factor = 0u;
        }
      }

      template<typename T>
      [[nodiscard]] T Lerp(T a, T b) const {
        if(b >= a) {
          return a + (b - a) * m_lerp_numer / m_lerp_denom;
        }
        return b + (a - b) * (m_lerp_denom - m_lerp_numer) / m_lerp_denom;
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
        if(m_use_lerp_fallback) {
          for(uint i = 0; i < n; i++) {
            out[i] = Lerp(a[i].Raw(), b[i].Raw());
          }
        } else {
          for(uint i = 0; i < n; i++) {
            out[i] = Perp(a[i].Raw(), b[i].Raw());
          }
        }
      }

    private:
      bool m_use_lerp_fallback{}; //< whether this is a 2D-polygon using linear interpolation for every attribute.

      u32 m_lerp_numer{};  //< linear interpolator numerator
      u32 m_lerp_denom{};  //< linear interpolation denominator
      u32 m_perp_factor{}; //< perspective-correct interpolation factor
  };

} // namespace dual::nds::gpu
