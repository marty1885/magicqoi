#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t* magicqoi_decode_mem_baseline(const uint8_t* data, size_t data_len, size_t* width, size_t* height, int* channels);
uint8_t* magicqoi_decode_stream_mem_baseline(const uint8_t* data, size_t data_len, size_t width, size_t height, int channels);
#ifdef __cplusplus
}
#endif
