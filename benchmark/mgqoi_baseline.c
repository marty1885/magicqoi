#include "mgqoi_baseline.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// for htohl
#include <arpa/inet.h>

// Fix ugly C keywords
#define alignof _Alignof
#define bool _Bool
#define static_assert _Static_assert
#define false 0
#define true 1

// QOI defines
#define QOI_OP_INDEX 0
#define QOI_OP_DIFF 1
#define QOI_OP_LUMA 2
#define QOI_OP_RUN_OR_RAW 3

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

struct _qoi_header
{
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colorspace;
} __attribute__((packed));
typedef struct _qoi_header qoi_header;

union _qoi_pixel
{
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } color;
    uint32_t word;
};
typedef union _qoi_pixel qoi_pixel;

struct _qoi_op_index {
    uint8_t index: 6;
    uint8_t tag: 2;
};
typedef struct _qoi_op_index qoi_op_index;

struct _qoi_op_diff {
    uint8_t db: 2;
    uint8_t dg: 2;
    uint8_t dr: 2;
    uint8_t tag: 2;
};
typedef struct _qoi_op_diff qoi_op_diff;

struct _qoi_op_run {
    uint8_t run: 6;
    uint8_t tag: 2;
};
typedef struct _qoi_op_run qoi_op_run;

typedef struct {
    uint8_t dg: 6;
    uint8_t tag: 2;
    uint8_t db_dg: 4;
    uint8_t dr_dg: 4;
} qoi_op_luma;

typedef struct {
    uint8_t tag;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} qoi_op_rgb;

typedef struct {
    uint8_t tag;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} qoi_op_rgba;

static bool is_big_endian()
{
    union {
        uint32_t i;
        char c[4];
    } e = { 0x01000000  };
    return e.c[0];

}

static bool get_qoi_header(qoi_header* header, const uint8_t* data, size_t data_len)
{
    const bool is_be = is_big_endian();

    if(data_len < sizeof(qoi_header))
        return false;
    if((size_t)data % alignof(qoi_header) != 0)
        return false;

    qoi_header h;
    memcpy(&h, data, sizeof(qoi_header));
    if(memcmp(h.magic, "qoif", 4) != 0)
        return false;
    if(h.channels != 3 && h.channels != 4)
        return false;
    if(h.colorspace != 0 && h.colorspace != 1)
        return false;

    if(!is_be) {
        h.width = ntohl(h.width);
        h.height = ntohl(h.height);
    }
    *header = h;
    return true;
}

static int qoi_pixel_hash(qoi_pixel pix)
{
    return (pix.color.r*3 + pix.color.g*5 + pix.color.b*7 + pix.color.a*11) % 64;
}

static int minof(int a, int b)
{
    return a < b ? a : b;
}

uint8_t* magicqoi_decode_mem_baseline(const uint8_t* data, size_t data_len, size_t* width, size_t* height, int* channels)
{
    qoi_header header;
    if(get_qoi_header(&header, data, data_len) == false)
        return NULL;

    const uint8_t* stream_ptr = data + sizeof(qoi_header);
    const size_t stream_size = data_len - sizeof(qoi_header);
    uint8_t* result = magicqoi_decode_stream_mem_baseline(stream_ptr, stream_size, header.width, header.height, header.channels);
    *width = header.width;
    *height = header.height;
    *channels = header.channels;
    return result;
}

uint8_t* magicqoi_decode_stream_mem_baseline(const uint8_t* data, size_t data_len, size_t width, size_t height, int channels)
{
    uint8_t* buffer = (uint8_t*)malloc(width * height * channels);
    qoi_pixel pixel_lut[64];
    memset(pixel_lut, 0, sizeof(pixel_lut));

    qoi_pixel last_pix;
    last_pix.color.r = 0;
    last_pix.color.g = 0;
    last_pix.color.b = 0;
    last_pix.color.a = 255;

    uint8_t* current_op = (uint8_t*)data;
    uint8_t* data_end = (uint8_t*)data + data_len;
    size_t write_pix = 0;
    int run_size = 1;
    while(current_op <= data_end) {
        const uint8_t op = *current_op;
        const uint8_t op_tag = (op & 0b11000000) >> 6;
        run_size = 1;
        if(op_tag == QOI_OP_RUN_OR_RAW) {
            if(op == 0b11111110) {
                if(data_end - current_op < sizeof(qoi_op_rgb)) {
                    fprintf(stderr, "decode error RGB %ld\n", current_op - data);
                    free(buffer);
                    return NULL;
                }
                static_assert(sizeof(qoi_op_rgb) == 4, "qoi_op_rgb size mismatch");
                qoi_op_rgb rgb_op;
                memcpy(&rgb_op, current_op, sizeof(rgb_op));
                //printf("RGB %d %d %d\n", rgb_op.r, rgb_op.g, rgb_op.b);
                current_op += sizeof(qoi_op_rgb);
                last_pix.color.r = rgb_op.r;
                last_pix.color.g = rgb_op.g;
                last_pix.color.b = rgb_op.b;
            } 
            else if(op == 0b11111111) {
                if(data_end - current_op < sizeof(qoi_op_rgba)) {
                    fprintf(stderr, "decode error RGBA %ld\n", current_op - data);
                    free(buffer);
                    return NULL;
                }
                static_assert(sizeof(qoi_op_rgba) == 5, "qoi_op_rgba size mismatch");
                qoi_op_rgba rgba_op;
                memcpy(&rgba_op, current_op, sizeof(rgba_op));
                //printf("RGBA %d %d %d %d\n", rgba_op.r, rgba_op.g, rgba_op.b, rgba_op.a);
                current_op += sizeof(qoi_op_rgba);
                last_pix.color.r = rgba_op.r;
                last_pix.color.g = rgba_op.g;
                last_pix.color.b = rgba_op.b;
                last_pix.color.a = rgba_op.a;
            } 
            else {
                qoi_op_run run_op;
                static_assert(sizeof(qoi_op_run) == 1, "qoi_op_run must have size 1");
                memcpy(&run_op, &op, 1);
                //printf("RUN %d\n", run_op.run);
                current_op += sizeof(qoi_op_run);
                run_size = run_op.run + 1;
            }
        }
        else if(op_tag == QOI_OP_INDEX) {
            qoi_op_index index_op;
            static_assert(sizeof(qoi_op_index) == 1, "qoi_op_index must have size 1");
            memcpy(&index_op, &op, 1);
            //printf("INDEX %d\n", index_op.index);
            current_op += sizeof(qoi_op_index);
            last_pix = pixel_lut[index_op.index];
        }
        else if(op_tag == QOI_OP_DIFF) {
            qoi_op_diff diff_op;
            static_assert(sizeof(qoi_op_diff) == 1, "qoi_op_diff must have size 1");
            memcpy(&diff_op, &op, 1);
            current_op += sizeof(qoi_op_diff);
            //printf("DIFF %d %d %d\n", diff_op.dr, diff_op.dg, diff_op.db);
            last_pix.color.r += diff_op.dr-2;
            last_pix.color.g += diff_op.dg-2;
            last_pix.color.b += diff_op.db-2;
        }
        else if(op_tag == QOI_OP_LUMA) {
            qoi_op_luma luma_op;
            static_assert(sizeof(qoi_op_luma) == 2, "qoi_op_luma must have size 2");
            if(data_end - current_op < sizeof(qoi_op_luma)) {
                fprintf(stderr, "decode error LUMA %ld\n", current_op - data);
                free(buffer);
                return NULL;
            }
            memcpy(&luma_op, current_op, 2);
            //printf("LUMA %d %d %d\n", luma_op.dg, luma_op.dr_dg, luma_op.db_dg);
            current_op += sizeof(qoi_op_luma);
            int real_dg = luma_op.dg - 32;
            last_pix.color.g += real_dg;
            last_pix.color.r += luma_op.dr_dg + real_dg - 8;
            last_pix.color.b += luma_op.db_dg + real_dg - 8;
        }

        for(int i=0;i<run_size;i++) {
            if(write_pix > width*height) {
                return buffer;
            }
            buffer[write_pix*channels+0] = last_pix.color.r;
            buffer[write_pix*channels+1] = last_pix.color.g;
            buffer[write_pix*channels+2] = last_pix.color.b;

            if(channels == 4)
                buffer[write_pix*channels+3] = last_pix.color.a;
            write_pix++;
        }
        int hash = qoi_pixel_hash(last_pix);
        pixel_lut[hash] = last_pix;
    }

    return buffer;
}