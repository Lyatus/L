#include <L/src/container/Buffer.h>
#include <L/src/rendering/Texture.h>
#include <L/src/stream/CFileStream.h>

#include <L/src/engine/Resource.inl>

L_PUSH_NO_WARNINGS

#define STB_DXT_IMPLEMENTATION
#define STB_DXT_STATIC
#include <stb_dxt.h>

L_POP_NO_WARNINGS

using namespace L;

static bool uses_alpha(const Texture::Intermediate& intermediate) {
  const uint8_t* pixel_data = (const uint8_t*)intermediate.mips[0].data();
  for(uintptr_t x = 0; x < intermediate.width; x++) {
    for(uintptr_t y = 0; y < intermediate.height; y++) {
      const uint8_t* p = pixel_data + (x + y * intermediate.width) * 4;
      if(p[3] != 0xff) {
        return true;
      }
    }
  }
  return false;
}

void stb_dxt_transformer(const ResourceSlot& slot, Texture::Intermediate& intermediate) {
  if(intermediate.format == RenderFormat::R8G8B8A8_UNorm &&
     intermediate.width % 4 == 0 &&
     intermediate.height % 4 == 0) {

    const bool alpha = uses_alpha(intermediate);
    const uintptr_t block_size_bytes(alpha ? 16 : 8);
    const uintptr_t block_size_pixels(16);
    intermediate.format = alpha ? RenderFormat::BC3_UNorm_Block : RenderFormat::BC1_RGB_UNorm_Block;

    for(uintptr_t mip = 0; mip < intermediate.mips.size(); mip++) {
      const uint32_t mip_width = intermediate.width >> mip;
      const uint32_t mip_height = intermediate.height >> mip;
      if(mip_width % 4 == 0 && mip_height % 4 == 0) {
        Buffer& uncompressed = intermediate.mips[mip];
        Buffer compressed = (mip_width * mip_height * block_size_bytes) / block_size_pixels;
        const uint32_t width_block(mip_width / 4), height_block(mip_height / 4);

        L_SCOPE_MARKER("stb_compress_dxt_block");
        for(uint32_t x(0); x < width_block; x++) {
          for(uint32_t y(0); y < height_block; y++) {
            uint8_t src_block[block_size_pixels * 4];
            const uint8_t* src((const uint8_t*)uncompressed.data() + (x * 4 + y * mip_width * 4) * 4);
            for(uint32_t row(0); row < 4; row++) {
              memcpy(src_block + row * 4 * 4, src + row * mip_width * 4, 4 * 4);
            }
            stb_compress_dxt_block((uint8_t*)compressed.data() + (x + y * width_block) * block_size_bytes, src_block, alpha, STB_DXT_HIGHQUAL);
          }
        }

        uncompressed = static_cast<Buffer&&>(compressed);
      } else if(mip_width < 4 || mip_height < 4) {
        // TODO: This is not correct but good enough for now
        intermediate.mips[mip] = Buffer(intermediate.mips[mip - 1]);
      }
    }
  }
}

void stb_dxt_module_init() {
  ResourceLoading<Texture>::add_transformer(stb_dxt_transformer, ResourceTransformPhase::VeryLate);
}
