#pragma once

#include <cstring>
#include "../math/digits.h"

namespace L {
  class Stream {
  public:
    virtual size_t write(const void* data,size_t size) = 0;
    virtual size_t read(void* data,size_t size) = 0;
    virtual char get() { char c; read(&c,1); return c; };
    virtual void put(char c) { write(&c,1); }
    virtual bool end() { return false; }

    const char* line(); // Reads a line until \n
    void line(char* buffer, size_t size); // Reads a line into buffer until \n or full
    const char* word(); // Reads a word until a space
    const char* bufferize(size_t*); // Reads as many chars as possible

    // Default operators
    template<class T> inline Stream& operator<<(const T&) { write("N/A",3); return *this; }

    template<class T> inline Stream& operator<<(T* v) { return *this << ntos<16>((uintptr_t)v,sizeof(v)*2); }

    inline Stream& operator<<(bool v) { if(v) write("true",4); else write("false",5); return *this; }
    inline Stream& operator<<(char v) { put(v); return *this; }
    inline Stream& operator<<(char* v) { write(v,strlen(v)); return *this; }
    inline Stream& operator<<(unsigned char v) { return *this << ntos<16>((unsigned int)v,2); }
    inline Stream& operator<<(const char* v) { write(v,strlen(v)); return *this; }
    inline Stream& operator<<(int16_t v) { return *this << ntos(v); }
    inline Stream& operator<<(uint16_t v) { return *this << ntos(v); }
    inline Stream& operator<<(int32_t v) { return *this << ntos(v); }
    inline Stream& operator<<(uint32_t v) { return *this << ntos(v); }
    inline Stream& operator<<(int64_t v) { return *this << ntos(v); }
    inline Stream& operator<<(uint64_t v) { return *this << ntos(v); }
    inline Stream& operator<<(float v) { return *this << ntos(v); }
    inline Stream& operator<<(double v) { return *this << ntos(v); }

    static inline bool isspace(char c) { return c==' '||c=='\t'||c=='\n'||c=='\v'||c=='\f'||c=='\r'; }
  };
}
