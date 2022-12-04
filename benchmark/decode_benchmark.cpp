#include <benchmark/benchmark.h>
#include <fstream>

#include <magicqoi.h>
#include "mgqoi_baseline.h" // non-optimized version of magicqoi

#define QOI_IMPLEMENTATION
#include "qoi.h"

using namespace benchmark;

std::vector<char> load_file(const std::string& path) {
    std::ifstream in(path);
    if(in.good() == false) {
        throw std::runtime_error("Pleast put " + path + " in the current directory");
    }
    return std::vector<char>( std::istreambuf_iterator<char>(in),
                                  std::istreambuf_iterator<char>{});
}
std::vector<std::vector<char>> buffer = {load_file("test.qoi")};

static void benchmarkDecodeOptimized(State& state) {
    for (auto _ : state) {
        for(auto& buf : buffer) {
            uint32_t width, height, channels;
            auto raw_pix = magicqoi_decode_mem((uint8_t*)buf.data(), buf.size(), &width, &height, &channels);
            DoNotOptimize(raw_pix);
            free(raw_pix);
        }
    }
}
BENCHMARK(benchmarkDecodeOptimized);

static void benchmarkDecodeNormal(State& state) {
    for (auto _ : state) {
        for(auto& buf : buffer) {
            size_t width, height;
            int channels;
            auto raw_pix = magicqoi_decode_mem_baseline((uint8_t*)buf.data(), buf.size(), &width, &height, &channels);
            DoNotOptimize(raw_pix);
            free(raw_pix);
        }
    }
}
BENCHMARK(benchmarkDecodeNormal);

static void benchmarkOfficalDecode(State& state) {
    for (auto _ : state) {
        for(auto& buf : buffer) {
            qoi_desc desc;
            auto pix = qoi_decode((uint8_t*)buf.data(), buf.size(), &desc, 0);
            DoNotOptimize(pix);
            QOI_FREE(pix);
        }
    }
}
BENCHMARK(benchmarkOfficalDecode);

int main(int argc, char** argv)
{
   ::benchmark::Initialize(&argc, argv);
   ::benchmark::RunSpecifiedBenchmarks();
}
