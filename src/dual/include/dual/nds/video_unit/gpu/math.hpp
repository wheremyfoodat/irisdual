
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
        constexpr FixedBase(T value) : m_value{value} {} // NOLINT(*-explicit-constructor)

        static constexpr FixedBase FromInt(int value) {
          return FixedBase{(T)((T)value << shift)};
        }

        [[nodiscard]] T Int() const {
          return m_value >> shift;
        }

        [[nodiscard]] T Raw() const {
          return m_value;
        }

        [[nodiscard]] FixedBase Abs() const {
          return m_value < 0 ? -m_value : m_value;
        }

        FixedBase operator+(FixedBase other) const {
          return m_value + other.m_value;
        }

        FixedBase operator-(FixedBase other) const {
          return m_value - other.m_value;
        }

        FixedBase operator*(FixedBase<T, U, shift> other) const {
          return T((U(m_value) * U(other.m_value)) >> shift);
        }

        FixedBase operator/(FixedBase other) const {
          return T((U(m_value) << shift) / U(other.m_value));
        }

        FixedBase& operator+=(FixedBase other) {
          m_value += other.m_value;
          return *this;
        }

        FixedBase& operator-=(FixedBase other) {
          m_value -= other.m_value;
          return *this;
        }

        FixedBase& operator*=(FixedBase other) {
          m_value = T((U(m_value) * U(other.m_value)) >> shift);
          return *this;
        }

        FixedBase& operator/=(FixedBase other) {
          m_value = T((U(m_value) << shift) / U(other.m_value));
          return *this;
        }

        FixedBase operator-() const {
          return -m_value;
        }

        bool operator==(FixedBase other) const {
          return m_value == other.m_value;
        }

        bool operator!=(FixedBase other) const {
          return m_value != other.m_value;
        }

        bool operator<=(FixedBase other) const {
          return m_value <= other.m_value;
        }

        bool operator>=(FixedBase other) const {
          return m_value >= other.m_value;
        }

        bool operator<(FixedBase other) const {
          return m_value < other.m_value;
        }

        bool operator>(FixedBase other) const {
          return m_value > other.m_value;
        }

      private:
        T m_value{};
    };

  } // namespace dual::nds::detail

  using Fixed20x12 = detail::FixedBase<i32, i64, 12>;
  using Fixed12x4 = detail::FixedBase<i16, i32, 4>;
  using Fixed6 = detail::FixedBase<i8, int, 6>;

  namespace detail {
    inline Fixed12x4 operator*(Fixed12x4 lhs, Fixed20x12 rhs) {
      return Fixed12x4{(i16)(Fixed20x12{lhs.Raw()} * rhs).Raw()};
    }

    inline Fixed6 operator*(Fixed6 lhs, Fixed20x12 rhs) {
      return Fixed6{i8((i64(lhs.Raw()) * rhs.Raw()) >> 12)};
    }
  } // namespace dual::nds::detail

} // namespace dual::nds

namespace atom {

  template<>
  struct NumericConstants<dual::nds::Fixed20x12> {
    static constexpr dual::nds::Fixed20x12 Zero() {
      return 0;
    }

    static constexpr dual::nds::Fixed20x12 One() {
      return dual::nds::Fixed20x12::FromInt(1);
    }
  };

  template<>
  struct NumericConstants<dual::nds::Fixed12x4> {
    static constexpr dual::nds::Fixed12x4 Zero() {
      return 0;
    }

    static constexpr dual::nds::Fixed12x4 One() {
      return dual::nds::Fixed12x4::FromInt(1);
    }
  };

  template<>
  struct NumericConstants<dual::nds::Fixed6> {
    static constexpr dual::nds::Fixed6 Zero() {
      return 0;
    }

    static constexpr dual::nds::Fixed6 One() {
      return dual::nds::Fixed6::FromInt(1);
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

  class Color4 : public atom::detail::Vector<Color4, Fixed6, 4> {
    public:
      Color4() {
        for(auto& component : this->data) component = 63;
      }

      Color4(Fixed6 r, Fixed6 g, Fixed6 b) {
        this->data[0] = r;
        this->data[1] = g;
        this->data[2] = b;
        this->data[3] = Fixed6{63};
      }

      Color4(Fixed6 r, Fixed6 g, Fixed6 b, Fixed6 a) {
        this->data[0] = r;
        this->data[1] = g;
        this->data[2] = b;
        this->data[3] = a;
      }

      Color4(const Color4& other) {
        operator=(other);
      }

      static Color4 FromRGB555(u16 color) {
        const int r = (color << 1) & 62;
        const int g = (color >> 4) & 62;
        const int b = (color >> 9) & 62;

        return Color4{
          (i8)(r | (r >> 5)),
          (i8)(g | (g >> 5)),
          (i8)(b | (b >> 5)),
          (i8)63
        };
      }

      [[nodiscard]] u16 ToRGB555() const {
        return  R().Raw() >> 1 |
               (G().Raw() >> 1) <<  5 |
               (B().Raw() >> 1) << 10;
      }

      Color4& operator=(const Color4& other) {
        for(const int i : {0, 1, 2, 3}) {
          this->data[i] = other[i];
        }
        return *this;
      }

      [[nodiscard]] Color4 operator*(Fixed6 other) const {
        Color4 result{};
        for(const int i : {0, 1, 2, 3}) {
          result[i] = data[i] * other;
        }
        return result;
      }

      Color4& operator*=(Fixed6 other) {
        for(const int i : {0, 1, 2, 3}) {
          data[i] *= other;
        }
        return *this;
      }

      Color4 operator*(Color4 const& other) const {
        Color4 result{};
        for(const int i : {0, 1, 2, 3}) {
          result[i] = data[i] * other[i];
        }
        return result;
      }

      Color4& operator*=(Color4 const& other) {
        for(const int i : {0, 1, 2, 3}) {
          data[i] *= other[i];
        }
        return *this;
      }

      [[nodiscard]] Fixed6& R() { return this->data[0]; }
      [[nodiscard]] Fixed6& G() { return this->data[1]; }
      [[nodiscard]] Fixed6& B() { return this->data[2]; }
      [[nodiscard]] Fixed6& A() { return this->data[3]; }

      [[nodiscard]] Fixed6 R() const { return this->data[0]; }
      [[nodiscard]] Fixed6 G() const { return this->data[1]; }
      [[nodiscard]] Fixed6 B() const { return this->data[2]; }
      [[nodiscard]] Fixed6 A() const { return this->data[3]; }
  };

} // namespace dual::nds
