# MagicQOI

MagicQOI is an optimized QOI codec written in C. To quote an random stranger on HackerNews

> The algorithm is extremely resistant to SIMD optimizations.
>
> Every pixel uses a different encoding, 95% of the encodings rely on the value of the previous pixel, or the accumulated state of all previously processed pixels. The number of bytes per pixel and pixels per byte swing wildly.
>
> SIMD optimization would basically require redesigning it from scratch. 
> - phire. 2021-11-15

That doesn't sit well with me. Formats like PNG cat get near QOI speed by exploiting SIMD on modern CPUs. Ok. maybe SIMD optimization is out of the discussion. Maybe some cache-friendly coding and and low level optimization when possible can still push QOI further. This project is wiritten in the trust your compiler but verify optimizer works approach.

As of writing this document. MagicQOI is at least as fast as the official QOI implementation. And up to 2x faster decoding images with lots of repitiions.

## Use

To build this project. You need a C11 capable compiler and CMake. The main API is `magicqoi_decode_mem` and `magicqoi_encode_mem` which encode/decodes the a QOI image given the raw data pointer. Additionally you need a C++17 compiler and Google Benchmark to build the benchmark and examples.

```c
// to decode
size_t width, height;
int channels;
uint8_t* raw_pixel = magicqoi_decode_mem(data, data_len, &width, &height, &channels);
assert(raw_pixel != NULL);
free(raw_pixel);

// to encode
size_t data_len;
uint8_t* qoi_data = magicqoi_encode_mem(raw_pixel, width, height, channels, &data_len);
assert(qoi_data != NULL);
free(qoi_data);
```

## Performance

MagicQOI is marginly faster then the official qoi.h (~10%) at decoding the official QOI image set. Up to more then 2x when facing simple images. Running on a NXP LX2160A (Cortex A72 @ 2.0GHz)

```
❯ ./benchmark/decode_set_benchmark /path/to/qoi/image/set

Loaded 2633 files. 1272MB total.
                decode:ms       decode:Mp/s     decode:Mb/s
magicqoi        13734           90.0035         92.6377
qoi             14985           82.4897         84.904

❯ ./benchmark/decode_benchmark
(decoding qoi_logo.qoi)
-------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations
-------------------------------------------------------------------
benchmarkDecodeOptimized     142227 ns       142000 ns         4930
benchmarkOfficalDecode       305718 ns       305262 ns         2293
```

The main performance gain comes from batching RLE pixel writes and adding a special fast path for it. Other minor performance gains comes from reducing load on L1i cache/branch prediction.

However. Performance comparsion to `rust-qoi` is unknown. I've ran both benchmarks on the same machine. Yet rust-qoi's qoi.h benchmark ran 2.5~3x the speed of ours under the same dataset. Take both numbers with a grain of salt.

## Roadmap

My current best guess of bottneck is the L1i or branch prediction. Evidence by the fact MagicQOI can hit 126Mp/s when decoding a stream of all `INDEX 0` operations. Also I haven't tested this on x64 CPUs.

- [x] QOI encoder
  - [ ] Optimize encoder
- [ ] Branchless decoding for QOI_OP_DIFF and QOI_OP_RUN (if possible)