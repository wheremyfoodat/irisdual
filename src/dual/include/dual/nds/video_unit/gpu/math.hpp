
#pragma once

#include <atom/math/traits.hpp>
#include <atom/math/matrix4.hpp>
#include <atom/math/vector.hpp>

#include <atom/integer.hpp>

namespace dual::nds {

  namespace detail {

    template<typename T, typename U, uint shift>
    class FixedBase {
      public:
        constexpr FixedBase() = default;
        constexpr FixedBase(T value) : value(value) {} // NOLINT(*-explicit-constructor)

        static FixedBase FromInt(int value) {
          return FixedBase{T{value} << shift};
        }

        [[nodiscard]] T Int() const {
          return value >> shift;
        }

        [[nodiscard]] T Raw() const {
          return value;
        }

        [[nodiscard]] FixedBase Abs() const {
          return value < 0 ? -value : value;
        }

        FixedBase operator+(FixedBase other) const {
          return value + other.value;
        }

        FixedBase operator-(FixedBase other) const {
          return value - other.value;
        }

        FixedBase operator*(FixedBase<T, U, shift> other) const {
          return T((U(value) * U(other.value)) >> shift);
        }

        FixedBase operator/(FixedBase other) const {
          return T((U(value) << shift) / U(other.value));
        }

        FixedBase& operator+=(FixedBase other) {
          value += other.value;
          return *this;
        }

        FixedBase& operator-=(FixedBase other) {
          value -= other.value;
          return *this;
        }

        FixedBase& operator*=(FixedBase other) {
          value = T((U(value) * U(other.value)) >> shift);
          return *this;
        }

        FixedBase& operator/=(FixedBase other) {
          value = T((U(value) << shift) / U(other.value));
          return *this;
        }

        FixedBase operator-() const {
          return -value;
        }

        bool operator==(FixedBase other) const {
          return value == other.value;
        }

        bool operator!=(FixedBase other) const {
          return value != other.value;
        }

        bool operator<=(FixedBase other) const {
          return value <= other.value;
        }

        bool operator>=(FixedBase other) const {
          return value >= other.value;
        }

        bool operator<(FixedBase other) const {
          return value < other.value;
        }

        bool operator>(FixedBase other) const {
          return value > other.value;
        }

      private:
        T value{};
    };

  } // namespace dual::nds::detail

  using Fixed20x12 = detail::FixedBase<s32, s64, 12>;
  using Fixed12x4 = detail::FixedBase<s16, s32, 4>;

  namespace detail {
    inline auto operator*(Fixed12x4 lhs, Fixed20x12 rhs) -> Fixed12x4 {
      return Fixed12x4{(s16)(Fixed20x12{lhs.Raw()} * rhs).Raw()};
    }
  } // namespace dual::nds::detail

} // namespace dual::nds

namespace atom {

  template<>
  struct NumericConstants<dual::nds::Fixed20x12> {
    static constexpr auto Zero() -> dual::nds::Fixed20x12 {
      return dual::nds::Fixed20x12{};
    }

    static constexpr auto One() -> dual::nds::Fixed20x12 {
      return dual::nds::Fixed20x12{1 << 12};
    }
  };

  template<>
  struct NumericConstants<dual::nds::Fixed12x4> {
    static constexpr auto Zero() -> dual::nds::Fixed12x4 {
      return dual::nds::Fixed12x4{};
    }

    static constexpr auto One() -> dual::nds::Fixed12x4 {
      return dual::nds::Fixed12x4{1 << 4};
    }
  };

} // namespace atom

namespace dual::nds {

  template<typename T>
  class Vector2 final : public atom::detail::Vector2<Vector2<T>, T> {
    public:
      using atom::detail::Vector2<Vector2<T>, T>::Vector2;
  };

  template<typename T>
  class Vector3 final : public atom::detail::Vector3<Vector3<T>, T> {
    public:
      using atom::detail::Vector3<Vector3<T>, T>::Vector3;
  };

  template<typename T>
  class Vector4 final : public atom::detail::Vector4<Vector4<T>, Vector3<T>, T> {
    public:
      using atom::detail::Vector4<Vector4<T>, Vector3<T>, T>::Vector4;
  };

  template<typename T>
  class Matrix4 final : public atom::detail::Matrix4<Matrix4<T>, Vector4<T>, T> {
    public:
      using atom::detail::Matrix4<Matrix4<T>, Vector4<T>, T>::Matrix4;
  };

} // namespace dual::nds
