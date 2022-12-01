#include <magicqoi.h>
#include <iostream>
#include <fstream>
#include <vector>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <chrono>

int main()
{
    std::ifstream in("test.qoi");
    if(in.good() == false) {
        std::cerr << "Cannot open test.qoi\n";
        return 1;
    }
    std::vector<char> buffer( std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>{});
    size_t width, height;
    int channels;

    auto raw_pix = magicqoi_decode_mem((uint8_t*)buffer.data(), buffer.size(), &width, &height, &channels);
    stbi_write_png("test.png", width, height, channels, raw_pix, width*channels);
    free(raw_pix);
    return 0;
}
