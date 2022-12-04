#ifdef __aarch64__
#define STBI_NEON
#elif defined(__x86_64__)
#define STBI_SSE2
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"