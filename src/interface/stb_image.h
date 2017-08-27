#pragma once

#include <L/src/L.h>
#define STB_IMAGE_IMPLEMENTATION 
#include <stb/stb_image.h>

namespace L {
  class STB_image : public Interface<Bitmap> {
    static STB_image instance;
  public:
    inline STB_image() : Interface{"png","bmp","jpeg","jpg"} {}
    Ref<Bitmap> from(const byte* data, size_t size) override {
      int width, height, comp;
      byte* img(stbi_load_from_memory(data, int(size), &width, &height, &comp, 4));
      if(img) {
        auto wtr = ref<Bitmap>(width, height);
        Bitmap& bmp = *wtr;
        size_t s(width*height);
        byte* a(img);
        byte* b((byte*)&bmp[0]);

        while(s--) {
          b[2] = a[0];
          b[1] = a[1];
          b[0] = a[2];
          b[3] = a[3];
          a += 4;
          b += 4;
        }
        stbi_image_free(img);
        return wtr;
      } else {
        out << stbi_failure_reason() << '\n';
        return nullptr;
      }
    }
  };
  STB_image STB_image::instance;
}
