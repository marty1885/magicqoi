#include <magicqoi.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main()
{
    std::ifstream in("test2.png");
    if(in.good() == false) {
        throw std::runtime_error("Cannot open test2.png");
    }
    std::vector<char> buffer( std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>{});
    int width, height, channels;
    auto raw_pix = stbi_load_from_memory((uint8_t*)buffer.data(), buffer.size(), &width, &height, &channels, 0);
    if(raw_pix == nullptr) {
        throw std::runtime_error("Cannot load test2.png");
    }

    size_t out_size;
    auto out_buf = magicqoi_encode_mem(raw_pix, width, height, channels, &out_size);
    if(out_buf == nullptr) {
        throw std::runtime_error("Cannot encode test.qoi");
    }

    std::ofstream out("test2.qoi");
    out.write((char*)out_buf, out_size);
    free(out_buf);
}