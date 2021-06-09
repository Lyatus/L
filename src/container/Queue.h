#pragma once

#include "../dev/debug.h"
#include "../math/math.h"

namespace L {
  template <intptr_t n, class T>
  class Queue {
    private:
      T _array[n] = {};
      intptr_t _w = 0;
      intptr_t _r = 0;

    public:
      constexpr Queue() {}
      inline intptr_t index(intptr_t i) const { return pmod(i, n); }
      inline bool full() const {return _w==index(_r-1);}
      inline bool empty() const {return _w==_r;}
      int size() const {return ((_r<=_w)?(_w-_r):(n-(_r-_w)));}
      void push(const T& e) {
        if(full()) error("Cannot push because static ring is full.");
        _array[_w] = e;
        _w = index(_w+1);
      }
      void pop() {
        if(empty()) error("Cannot pop because static ring is empty.");
        _r = index(_r+1);
      }
      const T& top() const {
        if(empty()) error("Cannot read because static ring is empty.");
        return _array[_r];
      }
  };
}
