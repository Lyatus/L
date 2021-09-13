#pragma once

#include "../macros.h"
#include "../text/String.h"
#include "../time/Time.h"
#include "../math/Vector.h"

namespace L {
  namespace System {
    int call(const char*, String& output); // Makes a system call and fills output
    int call(const char*); // Only makes a system call
    void sleep(int milliseconds);
    void sleep(const Time&);
    String pwd();

    String formatPath(String);
    String pathDirectory(const String&);
    String pathFile(const String&);

#if L_WINDOWS
    const char slash = '\\';
#elif L_LINUX
    const char slash = '/';
#endif

    inline void openURL(const String& url) {
#if L_WINDOWS
      call("start "+url);
#elif L_LINUX
      call("xdg-open "+url);
#endif
    }
  }
}
