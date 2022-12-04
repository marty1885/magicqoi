#include <magicqoi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <list>

namespace fs = std::filesystem;

#include "qoi.h"
#include "stb_image.h"

struct Image {
    std::vector<char> data;
    size_t width;
    size_t height;
    int channels;
};

Image load_file(const std::string& path)
{
    int width, height, channels;
    auto data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if(data == nullptr)
        throw std::runtime_error("Cannot decode " + path + " file using stb_image");
    auto res = Image{std::vector<char>(data, data + width*height*channels), (size_t)width, (size_t)height, channels};
    free(data);
    return res;
}

std::vector<size_t> encode_all_magicqoi(const std::list<Image>& buffers) {
    std::vector<size_t> results;
    for(auto& buf : buffers) {
        size_t out_size = 0;
        auto res = magicqoi_encode_mem((uint8_t*)buf.data.data(), buf.width, buf.height, buf.channels, &out_size);
        if(res == nullptr)
            continue;
        free(res);
        results.push_back(out_size);
    }
    return results;
}

std::vector<size_t> encode_all_qoi(const std::list<Image>& buffers) {
    std::vector<size_t> results;
    for(auto& buf : buffers) {
        qoi_desc desc;
        desc.width = buf.width;
        desc.height = buf.height;
        desc.channels = buf.channels;
        desc.colorspace = 0;
        int out_len = 0;
        auto res = qoi_encode((uint8_t*)buf.data.data(), &desc, &out_len);
        if(res == nullptr)
            continue;
        free(res);
        results.push_back(out_len);
    }
    return results;
}

int main(int argc, char** argv)
{
    std::list<Image> buffer;
    if(argc == 1) {
        std::cerr << argv[0] << " - MagicQOI image set encode benchmark tool\n";
        std::cerr << "Usage: " << argv[0] << " <path to directory containing PNG/JPG/BMP files for benchmark>\n";
        return 1;
    }
    std::cout << "Loading files from " << argv[1] << " ...\n";
    for (const auto & entry : fs::directory_iterator(argv[1])) {
        static const std::vector<std::string> extensions = {".png", ".jpg", ".bmp"};
        std::cout << "\33[2K\rLoading " << entry.path() << " ..." << std::flush;
        if(std::find(extensions.begin(), extensions.end(), entry.path().extension()) == extensions.end())
            continue;
        buffer.push_back(load_file(entry.path()));
    }
    size_t total_pixels = std::accumulate(buffer.begin(), buffer.end(), 0, [](size_t a, const Image& b) { return a + b.width*b.height; });
        size_t total_bytes = std::accumulate(buffer.begin(), buffer.end(), 0, [](size_t a, const Image& b) { return a + b.data.size(); });
    std::cout << "\n";
    std::cout << "Loaded " << buffer.size() << " files. " << total_pixels/1024/1024 << " MPixels total." << std::endl;
    std::cout << "\t\tencode:ms\tencode:Mp/s\tencode:Mb/s\tfin_size:MB\n";

    for(auto func : {encode_all_magicqoi, encode_all_qoi}) {
        auto start = std::chrono::high_resolution_clock::now();
        auto results = func(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << (func == encode_all_magicqoi ? "magicqoi" : "qoi.h\t") << "\t" << duration << "\t\t" << total_pixels/1024.0/duration
            << "\t\t" << total_bytes/1024.0/1024.0/duration*1000 << "\t\t" << total_bytes/1024.0/1024.0 << std::endl;
    }
    return 0;
}
