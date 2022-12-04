#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode a QOI image from memory
 * 
 * @param data The raw QOI byte stream to decode
 * @param data_len The length of the data in bytes
 * @param width [out] The width of the image
 * @param height [out] The height of the image
 * @param channels [out] The number of channels in the image
 * @return uint8_t* Pointer to the decoded image data. This must be freed with free(). NULL on error.
 */
uint8_t* magicqoi_decode_mem(const uint8_t* data, size_t data_len, uint32_t* width, uint32_t* height, uint32_t* channels);

/**
 * @brief Decodes a QOI stream from memory. 
 * @note This is not decoding a file. It decodes the raw stream. You need to know the width, height, and channels
 * of the image before you can decode it.
 * 
 * @param data The raw QOI byte stream to decode
 * @param data_len The length of the data in bytes
 * @param width Width of the image
 * @param height Height of the image
 * @param channels Channels of the image
 * @return uint8_t* Pointer to the decoded image data. This must be freed with free(). NULL on error.
 */
uint8_t* magicqoi_decode_stream_mem(const uint8_t* data, size_t data_len, uint32_t width, uint32_t height, uint32_t channels);

/**
 * @brief Encode an image to QOI format
 * 
 * @param data The raw image data to encode. Must be in RGB or RGBA format. Each component must be 8 bits.
 * @param width The width of the image
 * @param height The height of the image
 * @param channels The number of channels in the image 
 * @param out_len QOI encoded data length
 * @return uint8_t* Pointer to the encoded data. This must be freed with free(). NULL on error.
 */
uint8_t* magicqoi_encode_mem(const uint8_t* data, uint32_t width, uint32_t height, int channels, size_t* out_len);

/**
 * @brief Self test for internal functions. This function is for MagicQOI developers only.
 * 
 * @return int 0 if passed. Aborts if failed.
 */
int mgqoi_internal_self_test();

#ifdef __cplusplus
}
#endif
