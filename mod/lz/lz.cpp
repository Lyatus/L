#include <L/src/container/Buffer.h>
#include <L/src/macros.h>
#include <L/src/math/math.h>
#include <L/src/stream/CFileStream.h>
#include <L/src/text/compression.h>

using namespace L;

static size_t equal_until(const uint8_t* a, const uint8_t* b, size_t size) {
  // Number of bytes a and b share, size is max value
  if(size >= 8 && *(uint64_t*)a == *(uint64_t*)b) {
    size_t wtr(8);
    a += 8;
    b += 8;
    while(wtr < size && *a++ == *b++) {
      wtr++;
    }
    return wtr;
  } else {
    return 0;
  }
}
static const uint8_t* find_pattern(const uint8_t* haystack, const uint8_t* needle, size_t needle_size, size_t* pattern_size) {
  const uint8_t* pattern(nullptr);
  *pattern_size = 0;
  while((haystack + *pattern_size) < needle) {
    const size_t eq_size(equal_until(haystack, needle, min<size_t>(needle_size, needle - haystack)));
    if(eq_size > *pattern_size) {
      pattern = haystack;
      *pattern_size = eq_size;
    } else {
      haystack++;
    }
  }
  return pattern;
}
static void output_raw(const uint8_t* data, size_t size, Stream& out_stream) {
  while(size) {
    const uint16_t offset(0), chunk_size(uint16_t(min<size_t>(size, 0xffff)));
    out_stream.write(&offset, sizeof(offset));
    out_stream.write(&chunk_size, sizeof(chunk_size));
    out_stream.write(data, chunk_size);

    data += chunk_size;
    size -= chunk_size;
  }
}
static void lz_compress(const void* in_data_void, size_t in_size, Stream& out_stream) {
  const uint8_t* in_data_start((uint8_t*)in_data_void);
  const uint8_t* in_data(in_data_start);
  const uint8_t* in_data_needle(in_data);
  while(in_size > 0) {
    size_t pattern_size;
    if(const uint8_t* pattern = find_pattern(max(in_data_start, in_data_needle - 0x2000), in_data_needle, in_size, &pattern_size)) {
      if(in_data != in_data_needle) {
        output_raw(in_data, in_data_needle - in_data, out_stream);
      }
      {
        const uint16_t offset(uint16_t(in_data_needle - pattern)), size((uint16_t)pattern_size);
        L_ASSERT(!offset || size <= offset);
        out_stream.write(&offset, sizeof(offset));
        out_stream.write(&size, sizeof(size));
        in_data_needle += pattern_size;
        in_data = in_data_needle;
        in_size -= pattern_size;
      }
    } else {
      in_data_needle++;
      in_size--;
    }
  }
  if(in_data != in_data_needle) {
    output_raw(in_data, in_data_needle - in_data, out_stream);
  }
}

static Buffer lz_decompress(const void* in_data_void, size_t in_size) {
  const uint8_t* in_data((uint8_t*)in_data_void);
  const uint8_t* in_data_end(in_data + in_size);
  Buffer wtr;
  uintptr_t i(0);
  while(in_data < in_data_end) {
    uint16_t offset, size;
    offset = ((uint16_t*)in_data)[0]; // Endianness?
    size = ((uint16_t*)in_data)[1];

    if(wtr.size() < i + size) { // Grow buffer if necessary
      wtr.size(upperpow2(i + size));
    }

    const void* src(offset ? ((uint8_t*)wtr.data() + (i - offset)) : (in_data + 4));
    memcpy((uint8_t*)wtr.data() + i, src, size);
    i += size;

    const uintptr_t advance(offset ? 4 : size + 4);
    in_data += advance;
  }
  wtr.size(i);
  return wtr;
}

void lz_module_init() {
  register_compression({"lz", lz_compress, lz_decompress});
}
