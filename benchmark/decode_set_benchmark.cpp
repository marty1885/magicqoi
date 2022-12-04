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

std::vector<char> load_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if(in.good() == false) {
        throw std::runtime_error("Cannot open file");
    }
    return std::vector<char>( std::istreambuf_iterator<char>(in),
                                  std::istreambuf_iterator<char>{});
}

std::vector<size_t> decode_all_magicqoi(const std::list<std::vector<char>>& buffers) {
    std::vector<size_t> results;
    for(auto& buf : buffers) {
        uint32_t width, height, channels;
        auto raw_pix = magicqoi_decode_mem((uint8_t*)buf.data(), buf.size(), &width, &height, &channels);
        if(raw_pix == nullptr)
            throw std::runtime_error("Failed to decode a image");
        results.push_back(width*height);
        free(raw_pix);
    }
    return results;
}

std::vector<size_t> decode_all_qoi(const std::list<std::vector<char>>& buffers) {
    std::vector<size_t> results;
    for(auto& buf : buffers) {
        qoi_desc desc;
        auto pix = qoi_decode((uint8_t*)buf.data(), buf.size(), &desc, 0);
        if(pix == nullptr)
            throw std::runtime_error("Failed to decode a image");
        results.push_back(desc.width*desc.height);
        free(pix);
    }
    return results;
}

int main(int argc, char** argv)
{
    std::list<std::vector<char>> buffer;
    if(argc == 1) {
        std::cerr << argv[0] << " - MagicQOI image set benchmark tool\n";
        std::cerr << "Usage: " << argv[0] << " <path to directory containing QOI files for benchmark>\n";
        return 1;
    }
    std::cout << "Loading files from " << argv[1] << "...\n";
    for (const auto & entry : fs::directory_iterator(argv[1])) {
        if(entry.path().extension() == ".qoi")
            buffer.push_back(load_file(entry.path()));
    }
    size_t total_bytes = std::accumulate(buffer.begin(), buffer.end(), 0, [](size_t acc, const std::vector<char>& v) { return acc + v.size(); });
    std::cout << "Loaded " << buffer.size() << " files. " << total_bytes/1024/1024 << "MB total." << std::endl;
    std::cout << "\t\tdecode:ms\tdecode:Mp/s\tdecode:Mb/s\n";

    for(auto func : {decode_all_magicqoi, decode_all_qoi}) {
        auto start = std::chrono::high_resolution_clock::now();
        auto results = func(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        size_t total_pixels = std::accumulate(results.begin(), results.end(), 0);
        std::cout << (func == decode_all_magicqoi ? "magicqoi" : "qoi.h\t") << "\t" << duration << "\t\t" << total_pixels/1024.0/duration
            << "\t\t" << total_bytes/1024.0/1024.0/duration*1000 << std::endl;
    }
    return 0;
}