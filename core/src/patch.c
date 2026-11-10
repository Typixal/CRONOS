#include "ftota_patch.h"

#include <string.h>

#define OP_COPY 0x00
#define OP_LIT  0x02

/* Read 4 bytes as little-endian u32. Byte-by-byte, not a pointer cast:
 * a cast to uint32_t* can be misaligned and depends on host endianness.
 * This works the same on x86 host and ESP32-S3. */
static uint32_t read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* Minimal GREEN: first op only, no bounds checks yet.
 * Checks arrive test-first (T03 truncated, T04 bad base_off). */
ftota_status_t ftota_apply_segment(const uint8_t *base, size_t base_len,
                                   const uint8_t *seg, size_t seg_len,
                                   uint8_t *out, size_t out_len)
{
    (void)base_len; (void)seg_len; (void)out_len;

    if (seg[0] == OP_COPY) {
        uint32_t base_off = read_u32_le(seg + 1);
        uint32_t len      = read_u32_le(seg + 5);
        memcpy(out, base + base_off, len);
        return FTOTA_OK;
    }
    if (seg[0] == OP_LIT) {
        uint32_t len = read_u32_le(seg + 1);
        memcpy(out, seg + 5, len);
        return FTOTA_OK;
    }
    return FTOTA_ERR_UNIMPLEMENTED;
}
