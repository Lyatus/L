#pragma once

#include "../macros.h"
#include "../types.h"

#define L_ALLOCABLE(...) public: \
inline void* operator new(size_t SIZE) { L_ASSERT(SIZE==sizeof(__VA_ARGS__)); return L::Memory::alloc(sizeof(__VA_ARGS__));} \
inline void operator delete(void* P) { L::Memory::free(P,sizeof(__VA_ARGS__)); }

void* operator new(size_t size);

namespace L {
  class Memory {
  public:
    static void* alloc(size_t);
    static void* allocZero(size_t); // Allocates and zero-fill block
    static void* realloc(void*, size_t oldsize, size_t newsize);
    static void free(void*, size_t);

    static void* virtualAlloc(size_t);
    static void virtualFree(void*, size_t);
  };
}
