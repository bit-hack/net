#pragma once
#include <cstdint>

uint16_t fletcher16(const uint8_t *data, size_t bytes);
uint32_t adler32(const uint8_t *ptr, const size_t len);
uint64_t get_ticks();
