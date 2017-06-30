#pragma once

#include <cstdio>
#include <cstdlib>

#ifdef L_UNIX
# include <signal.h>
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define L_X86_64
#elif defined(__i386) || defined(_M_IX86)
#define L_X86_32
#endif

// Forbids class from being copied
#define L_NOCOPY(class) \
  class(const class&) = delete; \
  class& operator=(const class&) = delete;

// Allows true stringify
#define L_STRINGIFY(n) L_STRINGIFY_(n)
#define L_STRINGIFY_(n) #n

// Debugging macros
#ifdef L_DEBUG
#define L_DEBUGONLY(...) do{__VA_ARGS__;}while(false)
#else
#define L_DEBUGONLY(...) ((void)0)
#endif

#if defined _MSC_VER
# define L_BREAKPOINT __debugbreak()
#elif defined __GNUC__
# define L_BREAKPOINT raise(SIGTRAP)
#endif

#define L_ERROR(msg) do{fprintf(stderr,"Error in %s:%d:\n" msg "\n",__FILE__,__LINE__);L_BREAKPOINT;exit(0xA55E2737);}while(false)
#define L_ERRORF(msg,...) do{fprintf(stderr,"Error in %s:%d:\n" msg "\n",__FILE__,__LINE__,__VA_ARGS__);L_BREAKPOINT;exit(0xA55E2737);}while(false)
#define L_ASSERT(exp) L_DEBUGONLY(if(!(exp))L_ERRORF("%s is false",#exp))
#define L_ASSERT_MSG(exp,msg) L_DEBUGONLY(if(!(exp))L_ERROR(msg))

#define L_ONCE do{static bool DONE_ONCE(false);if(DONE_ONCE) return;DONE_ONCE = true;}while(false)
#define L_DO_ONCE static bool DONE_ONCE(false);if(!DONE_ONCE && (DONE_ONCE = true))

#define L_COUNT_OF(a) (sizeof(a)/sizeof(*a))
