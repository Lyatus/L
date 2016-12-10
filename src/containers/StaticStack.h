#pragma once

#include "../macros.h"
#include "../types.h"

namespace L {
  template <int n,class T>
  class StaticStack {
  private:
    byte _array[n*sizeof(T)];
    T* _current;

  public:
    StaticStack() : _current(((T*)_array)-1) {}
    template <class... Args>
    inline void push(Args&&... args) {
      _current++;
      L_ASSERT(_current<(T*)_array+n);
      new(_current)T(args...);
    }
    inline void pop() {
      L_ASSERT(_current>=(T*)_array);
      _current->~T();
      _current--;
    }
    inline T& top() { return *_current; }
    inline const T& top() const { return *_current; }
    inline T& top(int i){ return *(_current-i); }
    inline const T& top(int i) const { return *(_current-i); }
    inline T& bottom() { return *(T*)_array; }
    inline const T& bottom() const { return *(T*)_array; }
    inline T& bottom(int i){ return *((T*)_array+i); }
    inline const T& bottom(int i) const { return *((T*)_array+i); }
    inline T& operator[](int i) { return *(_current-i); }
    inline size_t size() const{ return _current+1-(const T*)_array; }
    inline void size(size_t s){
      while(s<size()) pop();
      while(s>size()) push();
    }
    inline bool empty() const { return (const T*)_array==_current+1; }
  };
}
