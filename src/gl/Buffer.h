#pragma once

#include "GL.h"
#include "../macros.h"

namespace L {
  namespace GL {
    class Buffer {
      L_NOCOPY(Buffer)
    private:
      GLuint _id,_target;
    public:
      Buffer(GLuint target);
      Buffer(GLuint target,GLsizeiptr size,const void* data,GLuint usage,GLuint base=-1);
      ~Buffer();

      void bind();
      void unbind();
      void data(GLsizeiptr size,const void* data,GLuint usage);
      void subData(GLintptr offset,GLsizeiptr size,const void* data);
      template<class T> void subData(GLintptr offset, const T& value) { subData(offset, sizeof(T), &value); }
      void bindBase(GLuint index);
    };
  }
}
