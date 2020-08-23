#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>

uint32_t adler32(const uint8_t *ptr, const size_t len) {
  static const uint32_t MOD_ADLER = 0xfff1;
  uint32_t a = 1, b = 0;
  const uint8_t *end = ptr + len;
  for (; ptr < end; ++ptr) {
    a = (a + *ptr) % MOD_ADLER;
    b = (b + a) % MOD_ADLER;
  }
  return (b << 16) | a;
}

uint16_t fletcher16(const uint8_t *data, size_t bytes) {
  uint16_t sum1 = 0xff, sum2 = 0xff;
  size_t tlen;
  while (bytes) {
    tlen = (bytes >= 20) ? 20 : bytes;
    bytes -= tlen;
    do {
      sum2 += sum1 += *data++;
    } while (--tlen);
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);
  }
  sum1 = (sum1 & 0xff) + (sum1 >> 8);
  sum2 = (sum2 & 0xff) + (sum2 >> 8);
  return sum2 << 8 | sum1;
}

uint64_t get_ticks() { return GetTickCount64(); }
