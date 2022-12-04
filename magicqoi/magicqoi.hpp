#pragma ocne

#include "magicqoi.h"

#include <stdlib.h>
#include <optional>
#include <string_view>
#include <vector>

namespace magicqoi {

static std::optional<std::vector<uint8_t>> encode(const uint8_t* data, uint32_t width, uint32_t height, uint32_t channels) {
    size_t out_len;
    auto encoded = magicqoi_encode_mem(data, width, height, channels, &out_len);
    if(encoded == nullptr) {
        return std::nullopt;
    }
    std::vector<uint8_t> ret(encoded, encoded + out_len);
    free(encoded);
}

static std::optional<std::vector<uint8_t>> decode(const uint8_t* data, size_t len, uint32_t* width, uint32_t* height, uint32_t* channels) {
    auto decoded = magicqoi_decode_mem(data, len, width, height, channels);
    if(decoded == nullptr) {
        return std::nullopt;
    }
    std::vector<uint8_t> ret(decoded, decoded + (*width) * (*height) * (*channels));
    free(decoded);
    return ret;
}

}