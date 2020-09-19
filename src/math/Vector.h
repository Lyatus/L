#pragma once

#include <cmath>
#include <limits>
#include <new>
#include "../math/math.h"
#include "../math/Rand.h"
#include "../stream/serial_text.h"

namespace L {
  template <size_t d,class T>
  class Vector {
  protected:
    T _c[d>0?d:1]; // Avoid zero-sized array
  public:
    inline Vector() = default;
    template <class R>
    inline Vector(const Vector<d,R>& other) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] = T(other[i]);
    }
    inline Vector(const T c[d]) {
      for(uintptr_t i(0); i < d; i++) {
        _c[i] = c[i];
      }
    }
    inline Vector(const T& v) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] = v;
    }
    template <typename... Args>
    inline Vector(const T& v,Args&&... args) {
      _c[0] = v;
      new(((T*)this)+1)Vector<d-1,T>(args...);
    }
    inline Vector(const Vector<d-1,T>& p,const T& w = 1.0) {
      for(uintptr_t i(0); i<d-1; i++)
        _c[i] = p[i];
      _c[d-1] = w;
    }
    inline Vector(const Vector<d+1,T>& p) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] = p[i];
    }

    Vector& operator+=(const Vector& other) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] += other._c[i];
      return *this;
    }
    Vector& operator-=(const Vector& other) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] -= other._c[i];
      return *this;
    }
    Vector& operator*=(const Vector& other) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] *= other._c[i];
      return *this;
    }
    Vector& operator*=(const T& scalar){
      for(auto& c : _c) c *= scalar;
      return *this;
    }
    Vector& operator/=(const Vector& other) {
      for(uintptr_t i(0); i<d; i++)
        _c[i] /= other._c[i];
      return *this;
    }
    inline Vector operator+(const Vector& other) const { return Vector(*this) += other; }
    inline Vector operator-(const Vector& other) const { return Vector(*this) -= other; }
    Vector operator-() const {
      Vector wtr;
      for(uintptr_t i(0); i<d; i++)
        wtr._c[i] = -_c[i];
      return wtr;
    }
    inline Vector operator*(const Vector& other) const { return Vector(*this) *= other; }
    inline Vector operator*(const T& scalar) const { return Vector(*this) *= scalar; }
    inline Vector operator/(const Vector& other) const { return Vector(*this) /= other; }

    bool operator==(const Vector& other) const {
      for(uintptr_t i(0); i<d; i++)
        if(_c[i] != other._c[i])
          return false;
      return true;
    }
    inline bool operator!=(const Vector& other) const { return !(*this == other); }
    bool operator<(const Vector& other) const {
      for(uintptr_t i(0); i<d; i++)
        if(_c[i] < other._c[i])
          return true;
        else if(_c[i] > other._c[i])
          return false;
      return false;
    }
    inline bool operator>(const Vector& other) const { return other<*this; }
    T length_squared() const {
      T wtr(0);
      for(uintptr_t i(0); i<d; i++)
        wtr += _c[i]*_c[i];
      return wtr;
    }
    inline T length() const { return std::sqrt(length_squared()); }
    inline Vector& length(const T& s){ return this->operator*=(s/length()); }
    T manhattan() const {
      T wtr(0);
      for(uintptr_t i(0); i<d; i++)
        wtr += L::abs(_c[i]);
      return wtr;
    }
    Vector& normalize() {
      T n(length());
      for(uintptr_t i(0); i<d; i++)
        _c[i] /= n;
      return *this;
    }
    inline Vector normalized() const { return Vector(*this).normalize(); }
    inline T dist(const Vector& other) const { return (*this-other).length(); }
    inline T dist_squared(const Vector& other) const { return (*this-other).length_squared(); }
    inline Vector cross(const Vector& other) const {
      return Vector(y()*other.z() - z()*other.y(),
                    z()*other.x() - x()*other.z(),
                    x()*other.y() - y()*other.x());
    }
    T dot(const Vector& other) const {
      T wtr(0);
      for(uintptr_t i(0); i<d; i++)
        wtr += _c[i]*other._c[i];
      return wtr;
    }
    T product() const {
      T wtr(1);
      for(uintptr_t i(0); i<d; i++)
        wtr *= _c[i];
      return wtr;
    }
    T sum() const {
      T wtr(0);
      for(uintptr_t i(0); i<d; i++)
        wtr += _c[i];
      return wtr;
    }
    inline Vector reflect(const Vector& v) const { return ((*this*(dot(v)*T(2)))-v); }

    inline const T& operator[](uintptr_t i) const { return _c[i]; }
    inline const T& x() const { return _c[0]; }
    inline const T& y() const { return _c[1]; }
    inline const T& z() const { return _c[2]; }
    inline const T& w() const { return _c[3]; }
    inline const uint8_t* bytes() const { return (const uint8_t*)&_c; }
    inline const T* array() const { return _c; }
    inline T& operator[](uintptr_t i) { return _c[i]; }
    inline T& x() { return _c[0]; }
    inline T& y() { return _c[1]; }
    inline T& z() { return _c[2]; }
    inline T& w() { return _c[3]; }

    static inline Vector min() { return Vector(std::numeric_limits<T>::min()); }
    static inline Vector max() { return Vector(std::numeric_limits<T>::max()); }
    static Vector random() {
      Vector wtr;
      for(uintptr_t i(0); i<d; i++)
        wtr[i] = Rand::next_float()-.5f;
      wtr.normalize();
      return wtr;
    }

    friend Stream& operator<<(Stream &s, const Vector& v) {
      s << '(';
      for(uintptr_t i(0); i<d; i++) {
        s << v[i];
        if(i<d-1)
          s << ';';
      }
      s << ')';
      return s;
    }
    friend inline Stream& operator<(Stream& s, const Vector& v) { for(uintptr_t i(0); i<d; i++) s < v[i]; return s; }
    friend inline Stream& operator>(Stream &s, Vector& v) { for(uintptr_t i(0); i<d; i++) s > v[i]; return s; }
  };
  template <size_t d,class T> inline Vector<d,T> operator*(const T& a,const Vector<d,T>& b) { return b*a; }

  typedef Vector<4, uint8_t> Vector4b;
  typedef Vector<2, int32_t> Vector2i;
  typedef Vector<3, int32_t> Vector3i;
  typedef Vector<4, int32_t> Vector4i;
  typedef Vector<2, uint32_t> Vector2ui;
  typedef Vector<3, uint32_t> Vector3ui;
  typedef Vector<4, uint32_t> Vector4ui;
  typedef Vector<2, float> Vector2f;
  typedef Vector<3, float> Vector3f;
  typedef Vector<4, float> Vector4f;

  template <size_t d,class T>
  Vector<d,T> clamp(const Vector<d,T>& v,const Vector<d,T>& min,const Vector<d,T>& max) {
    Vector<d,T> wtr;
    for(uintptr_t i(0); i<d; i++)
      wtr[i] = clamp(v[i],min[i],max[i]);
    return wtr;
  }
  template <size_t d,class T>
  Vector<d,T> abs(const Vector<d,T>& v) {
    Vector<d,T> wtr;
    for(uintptr_t i(0); i<d; i++)
      wtr[i] = abs(v[i]);
    return wtr;
  }
}
