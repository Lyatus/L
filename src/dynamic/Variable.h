#pragma once

#include "Cast.h"
#include "Type.h"

namespace L {
  class Variable {
    private:
      union {
        char _data[12];
        void* _p;
      };
      const TypeDescription* _td;

      inline void* value(){ return (local()) ? (void*)&_data : _p; }
      inline const void* value() const{ return (local()) ? (void*)&_data : _p; }

    public:
      inline Variable() : _td(Type<int>::description()) {}
      template <class T> Variable(const T& v) : _td(Type<T>::description()) {
        if(local())  // Value is to be contained locally
          new(&_data) T(v);
        else _p = new T(v);
      }
      Variable(const char*);
      Variable(const Variable&);
      Variable(Variable&&);
      Variable& operator=(const Variable&);
      Variable& operator=(Variable&&);
      ~Variable();

      inline const TypeDescription* type() const {return _td;}
      inline bool local() const {return _td->size<=sizeof(_data);}

      template <class T> inline bool is() const {return _td == Type<T>::description();}
      template <class T> inline const T& as() const {return *(T*)value();}
      template <class T> inline T& as() {return *(T*)value();}

      inline bool canbe(TypeDescription* td) const {return (Cast::get(_td, td) != nullptr);}
      template <class T> inline bool canbe() const {return canbe(Type<T>::description());}

      template <class T> T& make() {
        if(is<T>()) return as<T>(); // Return as-is
        else if(CastFct cast = Cast::get(_td,Type<T>::description())) { // Try to find cast 
          Var tmp;
          cast(*this,tmp);
          swap(*this,tmp);
          return as<T>();
        }
        else{ // Default construct it
          if(local()) new(&_data) T();
          else _p = new T();
          _td = Type<T>::description();
          return as<T>();
        }
      }
      template <class T> T get() const {
        if(is<T>()) return as<T>();
        else if(CastFct cast = Cast::get(_td,Type<T>::description())) {
          Var tmp;
          cast(*this,tmp);
          return tmp.as<T>();
        } else return T();
      }

      inline bool operator==(const Variable& other) const {return (_td == other._td && _td->hascmp())?(_td->cmp(value(),other.value()) == 0):false;}
      inline bool operator!=(const Variable& other) const {return !(*this == other);}
      inline bool operator>(const Variable& other) const {return (_td == other._td && _td->hascmp())?(_td->cmp(value(),other.value()) > 0):(_td>other._td);}
      inline bool operator<(const Variable& other) const {return (_td == other._td && _td->hascmp())?(_td->cmp(value(),other.value()) < 0):(_td<other._td);}
      inline bool operator>=(const Variable& other) const {return !(*this < other);}
      inline bool operator<=(const Variable& other) const {return !(*this > other);}

      Variable& operator[](const String&);
      const Variable& operator[](const String&) const;
      Variable& operator[](size_t);

      template<class T> inline operator T() {return get<T>();}

      friend Stream& operator<<(Stream&, const Variable&);
  };
  Stream& operator<<(Stream&, const Variable&);
  typedef Variable Var;
}
