#pragma once

#include "../container/Table.h"
#include "../stream/serial_text.h"
#include "../text/Symbol.h"
#include "../hash.h"

namespace L {
  typedef void(*Cast)(void*,const void*);
  // Structure to keep basic functions for a type
  struct TypeDescription{
    // Mandatory
    Symbol name;
    size_t size;

    void(*ctr)(void*);
    void* (*ctrnew)();
    void* (*cpy)(const void*);
    void(*cpyto)(void*,const void*);
    void(*assign)(void*,const void*);
    void(*dtr)(void*);
    void(*del)(void*);
    void(*print)(Stream&, const void*);
    void(*out_text)(Stream&, const void*);
    void(*in_text)(Stream&, void*);
    void(*out_bin)(Stream&, const void*);
    void(*in_bin)(Stream&, void*);
    uint32_t(*hash)(const void*);

    // Optional
    void(*add)(void*,const void*);
    void(*sub)(void*,const void*);
    void(*mul)(void*,const void*);
    void(*div)(void*,const void*);
    void(*mod)(void*,const void*);
    void(*inv)(void*);

    int(*cmp)(const void*,const void*);

    Table<const TypeDescription*, Cast> casts;
  };

  template <typename T>
  Symbol type_name() {
    static Symbol wtr;
    if(!wtr) {
#if defined _MSC_VER
      // "class L::Symbol __cdecl L::type_name<int>(void)"
      // "class L::Symbol __cdecl L::type_name<class L::String>(void)"
      char funcsig[] = __FUNCSIG__;
      const uintptr_t start(sizeof("class L::Symbol __cdecl L::type_name<")-1);
      const uintptr_t end(strlen(funcsig)-sizeof(">(void)")+1);
      funcsig[end] = '\0';
      char* name(funcsig+start);
#else
      // "L::Symbol L::type_name() [with T = XXX]"
      char tmp[256];
      strcpy(tmp, __PRETTY_FUNCTION__+sizeof("L::Symbol L::type_name() [with T = ")-1);
      tmp[strlen(tmp)-1] = '\0';
      char* name(tmp);
#endif
      const char* subs[] ={ "class ","struct ", " " };
      for(const char* sub : subs) {
        while(char* found = strstr(name, sub)) {
          const size_t sub_len = strlen(sub);
          memmove(found, found + sub_len, strlen(found) - sub_len + 1);
        }
      }
      wtr = name;
    }
    return wtr;
  }

  extern Table<Symbol, const TypeDescription*> types;
  template <class T>
  class Type {
  private:
    static TypeDescription td;
    static TypeDescription make_desc() {
      TypeDescription wtr = {
        type_name<T>(),sizeof(T),
        ctr,ctrnew,cpy,cpyto,assign,dtr,del,
        print,out_text,in_text,out_bin,in_bin,
        Type<T>::hash,0
      };
      types[wtr.name] = &td;
      return wtr;
    }
    static void ctr(void* p) { ::new(p)T(); }
    static void* ctrnew() { return new T; }
    static void* cpy(const void* p) { return new T(*(const T*)p); }
    static void cpyto(void* dst,const void* src) { ::new((T*)dst) T(*(const T*)src); }
    static void assign(void* dst,const void* src) { *(T*)dst = *(const T*)src; }
    static void dtr(void* p) { (void)p; ((T*)p)->~T(); }
    static void del(void* p) { delete(T*)p; }
    static void print(Stream& s,const void* p) { s << (*(const T*)p); }
    static void out_text(Stream& s, const void* p) { s < (*(const T*)p); }
    static void in_text(Stream& s, void* p) { s > (*(T*)p); }
    static void out_bin(Stream& s, const void* p) { s <= (*(const T*)p); }
    static void in_bin(Stream& s, void* p) { s >= (*(T*)p); }
    static uint32_t hash(const void* p) { return L::hash(*(const T*)p); }

  public:
    static inline const TypeDescription* description() { return &td; }
    static inline const char* name() { return td.name; }

    // Casts
    static inline void addcast(const TypeDescription* otd, Cast cast) { td.casts[otd] = cast; }
    template <class R> static inline void addcast(Cast cast){ addcast(Type<R>::description(),cast); }
    template <class R> static inline void addcast(){ addcast<R>([](void* dst,const void* src){new(dst)R(R(*(T*)src)); }); }

    // Operator setters
    static inline void useadd(void(*add)(void*,const void*)){ td.add = add; }
    static inline void usesub(void(*sub)(void*,const void*)){ td.sub = sub; }
    static inline void usemul(void(*mul)(void*,const void*)){ td.mul = mul; }
    static inline void usediv(void(*div)(void*,const void*)){ td.div = div; }
    static inline void usemod(void(*mod)(void*,const void*)){ td.mod = mod; }
    static inline void useinv(void(*inv)(void*)){ td.inv = inv; }
    static inline void usecmp(int(*cmp)(const void*,const void*)){ td.cmp = cmp; }

    // Operator default setters
    template <class dummy = void> static inline void canall(){ cancmp<>(); canmath<>(); canmod<>(); }
    template <class dummy = void> static inline void canumath(){ canadd<>(); cansub<>(); canmul<>(); candiv<>(); }
    template <class dummy = void> static inline void canmath(){ canumath<>(); caninv<>(); }
    template <class dummy = void> static inline void canadd(){ useadd([](void* a,const void* b) { *((T*)a) += *((T*)b); }); }
    template <class dummy = void> static inline void cansub(){ usesub([](void* a,const void* b) { *((T*)a) -= *((T*)b); }); }
    template <class dummy = void> static inline void canmul(){ usemul([](void* a,const void* b) { *((T*)a) *= *((T*)b); }); }
    template <class dummy = void> static inline void candiv(){ usediv([](void* a,const void* b) { *((T*)a) /= *((T*)b); }); }
    template <class dummy = void> static inline void canmod(){ usemod([](void* a,const void* b) { *((T*)a) %= *((T*)b); }); }
    template <class dummy = void> static inline void caninv(){ useinv([](void* a) { *((T*)a) = -*((T*)a); }); }
    template <class dummy = void> static inline void cancmp(){
      usecmp([](const void* a,const void* b)->int {
        if((*(T*)a)<(*(T*)b)) {
          return -1;
        } else if((*(T*)b)==(*(T*)a)) {
          return 0;
        }
        return 1;
      });
    }
  };
  template <>
  inline TypeDescription Type<void>::make_desc() {
    TypeDescription wtr = {
      "void",0,
      [](void*) {},
      []() -> void* { return nullptr; },
      [](const void*) -> void* { return nullptr; },
      [](void*, const void*) {},
      [](void*, const void*) {},
      [](void*) {},
      [](void*) {},
      [](Stream&, const void*) {},
      [](Stream&, const void*) {},
      [](Stream&, void*) {},
      [](Stream&, const void*) {},
      [](Stream&, void*) {},
      [](const void*) -> uint32_t { return 0; }
    };
    return wtr;
  }

  // Instantiate structures
  template <class T> TypeDescription Type<T>::td(Type<T>::make_desc());

  void init_type();
}
