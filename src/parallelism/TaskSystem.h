#pragma once

#include <cstdint>
#include "../macros.h"

namespace L {
  namespace TaskSystem {
    enum Flags {
      NoParent = 1<<1,
      MainTask = 1<<2,
    };
    typedef void(*Func)(void*);
    typedef bool(*CondFunc)(void*);

    void init();
    uint32_t thread_count();
    uint32_t thread_id();
    uint32_t fiber_count();
    uint32_t fiber_id();
    void push(Func, void* = nullptr, uint32_t thread_mask = -1, uint32_t flags = 0);
    void yield();
    void yield_until(CondFunc, void* = nullptr);
    void join();
    void join_all();

    uint32_t thread_mask();
    void thread_mask(uint32_t mask);
  }

#define L_SCOPE_THREAD_MASK(mask) L::ScopeThreadMask L_CONCAT(THREAD_MASK_,__LINE__)(mask)
  class ScopeThreadMask {
  protected:
    const uint32_t _original_mask;
  public:
    inline ScopeThreadMask(uint32_t mask) : _original_mask(TaskSystem::thread_mask()) { TaskSystem::thread_mask(mask); }
    inline ~ScopeThreadMask() { TaskSystem::thread_mask(_original_mask); }
  };
}
