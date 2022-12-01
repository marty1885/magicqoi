#include "magicqoi.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// for htohl
#include <arpa/inet.h>

// Fix ugly C keywords
#define alignof _Alignof
#define bool _Bool
#define static_assert _Static_assert
#define false 0
#define true 1
#define is_aligned(POINTER, BYTE_COUNT) \
    (((uintptr_t)(const void *)(POINTER)) % (BYTE_COUNT) == 0)


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

static int qoi_pixel_hash(qoi_pixel pix)
{
    return (pix.color.r*3 + pix.color.g*5 + pix.color.b*7 + pix.color.a*11) % 64;
}

static int minof(int a, int b)
{
    return a < b ? a : b;
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

uint8_t* magicqoi_decode_mem(const uint8_t* data, size_t data_len, size_t* width, size_t* height, int* channels)
{
    qoi_header header;
    if(get_qoi_header(&header, data, data_len) == false)
        return NULL;

    const uint8_t* stream_ptr = data + sizeof(qoi_header);
    const size_t stream_size = data_len - sizeof(qoi_header);
    uint8_t* result = magicqoi_decode_stream_mem(stream_ptr, stream_size, header.width, header.height, header.channels);
    *width = header.width;
    *height = header.height;
    *channels = header.channels;
    return result;
}

uint8_t* magicqoi_decode_stream_mem(const uint8_t* data, size_t data_len, size_t width, size_t height, int channels)
{
    uint8_t* buffer = (uint8_t*)malloc(width * height * channels);
    qoi_pixel pixel_lut[64];
    memset(pixel_lut, 0, sizeof(pixel_lut));

    // Who knows, the register keyword is helpful for once
    register qoi_pixel last_pix;
    last_pix.color.r = 0;
    last_pix.color.g = 0;
    last_pix.color.b = 0;
    last_pix.color.a = 255;

    const int op_size[4] = {1, 2, 4, 5 };
    uint8_t* current_op = (uint8_t*)data;
    const uint8_t* data_end = (uint8_t*)data + data_len;
    size_t write_pix = 0;

    while(unlikely(current_op < data_end && write_pix < width*height)) {
        const uint8_t op = *current_op;
        const uint8_t op_tag = op >> 6;
        const uint8_t op_data = op & 0b00111111;

        // Pre-handle failure cases to reduce branching later
        const size_t remaining = data_end - current_op;
        const int op_size_idx = (op_tag == QOI_OP_LUMA) + (op == 0xfe) * 2 + (op == 0xff) * 3;
        if(unlikely(remaining < op_size[op_size_idx])) {
            free(buffer);
            return NULL;
        }

        if(op_tag == QOI_OP_RUN_OR_RAW) {
            if(op == 0b11111110) {
                static_assert(sizeof(qoi_op_rgb) == 4, "qoi_op_rgb size mismatch");
                qoi_op_rgb rgb_op;
                memcpy(&rgb_op, current_op, sizeof(rgb_op));

                last_pix.color.r = current_op[1];
                last_pix.color.g = current_op[2];
                last_pix.color.b = current_op[3];
                current_op += sizeof(qoi_op_rgb);
            } 
            else if(op == 0b11111111) {
                static_assert(sizeof(qoi_op_rgba) == 5, "qoi_op_rgba size mismatch");
                last_pix.color.r = current_op[1];
                last_pix.color.g = current_op[2];
                last_pix.color.b = current_op[3];
                last_pix.color.a = current_op[4];
                current_op += sizeof(qoi_op_rgba);
            } 
            else {
                static_assert(sizeof(qoi_op_run) == 1, "qoi_op_run must have size 1");
                current_op += sizeof(qoi_op_run);
                int run_size = op_data + 1;

                // Batch pixel writes. Overall 0.4% speedup
                while(likely(current_op < data_end)) {
                    const uint8_t op = *current_op;
                    const uint8_t run_tag = op >> 6;
                    if(run_tag != QOI_OP_RUN_OR_RAW || op != 0b11111111 || op != 0b11111110)
                        break;
                    qoi_op_run run_op;
                    (*(char*)&run_op) = op;
                    current_op += sizeof(qoi_op_run);
                    run_size += run_op.run + 1;
                }

                size_t write_size = minof(run_size, width*height - write_pix);
                // fast path for runs of the same color
                if(channels == 4) {
                    for(size_t i=0; i<write_size; i++)
                        ((int*)buffer)[write_pix+i] = last_pix.word;
                    write_pix += write_size;
                }
                else {
                    for(size_t i=0;i<write_size; i++) {
                        buffer[(write_pix+i)*channels+0] = last_pix.color.r;
                        buffer[(write_pix+i)*channels+1] = last_pix.color.g;
                        buffer[(write_pix+i)*channels+2] = last_pix.color.b;
                    }
                    write_pix += write_size;
                }
                // No need to update the hash table, since we're just repeating the same color
                // int hash = qoi_pixel_hash(last_pix);
                // pixel_lut[hash] = last_pix;
                continue;
            }
        }
        else if(op_tag == QOI_OP_INDEX) {
            static_assert(sizeof(qoi_op_index) == 1, "qoi_op_index must have size 1");
            current_op += sizeof(qoi_op_index);
            last_pix = pixel_lut[op_data];
        }
        else if(op_tag == QOI_OP_DIFF) {
            qoi_op_diff diff_op;
            static_assert(sizeof(qoi_op_diff) == 1, "qoi_op_diff must have size 1");
            (*(char*)&diff_op) = op;
            current_op += sizeof(qoi_op_diff);
            last_pix.color.r += diff_op.dr-2;
            last_pix.color.g += diff_op.dg-2;
            last_pix.color.b += diff_op.db-2;
        }
        else /*if(op_tag == QOI_OP_LUMA)*/ {
            static_assert(sizeof(qoi_op_luma) == 2, "qoi_op_luma must have size 2");
            uint8_t next_byte = current_op[1];
            current_op += sizeof(qoi_op_luma);
            int dr_dg = (next_byte >> 4);
            int db_dg = (next_byte & 0b00001111);
            int real_dg = op_data - 32;
            last_pix.color.g += real_dg;
            last_pix.color.r += dr_dg + real_dg - 8;
            last_pix.color.b += db_dg + real_dg - 8;
        }

        if(channels == 4)
            ((int*)buffer)[write_pix] = last_pix.word;
        else {
            buffer[write_pix*channels+0] = last_pix.color.r;
            buffer[write_pix*channels+1] = last_pix.color.g;
            buffer[write_pix*channels+2] = last_pix.color.b;
        }
        write_pix++;

        int hash = qoi_pixel_hash(last_pix);
        pixel_lut[hash] = last_pix;
    }

    if(current_op >= data_end && write_pix < width*height) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

#define MAGICQOI_ASSERT(x) if(!(x)) { fprintf(stderr, "assertion failed: %s\n", #x); abort(); }

int mgqoi_internal_self_test()
{
    char op;

    op = 0b11111101;
    qoi_op_run run_op;
    memcpy(&run_op, &op, 1);
    MAGICQOI_ASSERT(run_op.tag == 0b11);
    MAGICQOI_ASSERT(run_op.run == 0b111101);

    op = 0b00000010;
    qoi_op_index index_op;
    memcpy(&index_op, &op, 1);
    MAGICQOI_ASSERT(index_op.tag == 0b00);
    MAGICQOI_ASSERT(index_op.index == 0b000010);

    op = 0b01110110;
    qoi_op_diff diff_op;
    memcpy(&diff_op, &op, 1);
    MAGICQOI_ASSERT(diff_op.tag == 0b01);
    MAGICQOI_ASSERT(diff_op.dr == 0b11);
    MAGICQOI_ASSERT(diff_op.dg == 0b01);
    MAGICQOI_ASSERT(diff_op.db == 0b10);

    char op2[2] = {0b10000001, 0b10000010};
    qoi_op_luma luma_op;
    memcpy(&luma_op, op2, 2);
    MAGICQOI_ASSERT(luma_op.tag == 0b10);
    MAGICQOI_ASSERT(luma_op.dg == 0b00000001);
    MAGICQOI_ASSERT(luma_op.dr_dg == 0b1000);
    MAGICQOI_ASSERT(luma_op.db_dg == 0b0010);

    char op4[4] = {0b11111110, 0xf1, 0x2f, 0x3f};
    qoi_op_rgb rgb_op;
    memcpy(&rgb_op, op4, 4);
    MAGICQOI_ASSERT(rgb_op.tag == 0b11111110);
    MAGICQOI_ASSERT(rgb_op.r == 0xf1);
    MAGICQOI_ASSERT(rgb_op.g == 0x2f);
    MAGICQOI_ASSERT(rgb_op.b == 0x3f);

    return 0;
}