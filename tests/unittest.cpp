#include <magicqoi.h>

#include <random>
#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include <cstring>

TEST_CASE("Random QOI encode decode")
{
    std::vector<uint8_t> raw_pix(256 * 256 * 4);
    std::mt19937 gen(0);
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for(auto& p : raw_pix) {
        p = dis(gen);
    }

    size_t out_size;
    auto out_buf = magicqoi_encode_mem(raw_pix.data(), 256, 256, 4, &out_size);
    REQUIRE(out_buf != nullptr);

    uint32_t width, height, channels;
    auto decoded = magicqoi_decode_mem(out_buf, out_size, &width, &height, &channels);
    REQUIRE(decoded != nullptr);

    REQUIRE(width == 256);
    REQUIRE(height == 256);
    REQUIRE(channels == 4);
    CHECK(memcmp(raw_pix.data(), decoded, raw_pix.size()) == 0);

    free(out_buf);
    free(decoded);
}

TEST_CASE("Good minimal QOI decode") {
    // 100x100 image with 3 channels green image
    std::vector<uint8_t> data = {0x71, 0x6f, 0x69, 0x66, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x03, 0x00, 0xfe, 0x3f, 0xca, 0xb8, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd
    , 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd
    , 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd
    , 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd
    , 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd
    , 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd
    , 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint32_t width, height, channels;
    auto decoded = magicqoi_decode_mem(data.data(), data.size(), &width, &height, &channels);
    REQUIRE(decoded != nullptr);
    free(decoded);
}

TEST_CASE("Bad QOI data") {
    // same data as above but too short
    std::vector<uint8_t> data = {0x71, 0x6f, 0x69, 0x66, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x03, 0x00, 0xfe};
    uint32_t width, height, channels;
    auto decoded = magicqoi_decode_mem(data.data(), data.size(), &width, &height, &channels);
    REQUIRE(decoded == nullptr);
}

int main(int argc, char** argv)
{
    // Make sure internal tests work before running Catch2
    mgqoi_internal_self_test();

    // start catch2
    int result = Catch::Session().run(argc, argv);
    return result;
}