#include "ftota_patch.h"

#include <string.h>

#define OP_COPY 0x00
#define OP_ADD  0x01
#define OP_LIT  0x02

#define COPY_HEADER_SIZE 9   /* op byte + u32 base_off + u32 len */
#define ADD_HEADER_SIZE  9   /* op byte + u32 base_off + u32 len, then diff */
#define LIT_HEADER_SIZE  5   /* op byte + u32 length */

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

/* GREEN so far: LIT header+data, COPY header and ADD header lengths are
 * checked (T03). ADD diff, base range and output checks arrive test-first. */
ftota_status_t ftota_apply_segment(const uint8_t *base, size_t base_len,
                                   const uint8_t *seg, size_t seg_len,
                                   uint8_t *out, size_t out_len)
{
    (void)base_len; (void)out_len;

    size_t seg_pos = 0;   /* next unread byte in seg */
    size_t out_pos = 0;   /* next unwritten byte in out */

    while (seg_pos < seg_len) {
        uint8_t op = seg[seg_pos];
        size_t remaining = seg_len - seg_pos;   /* bytes left, op byte incl. */

        if (op == OP_COPY) {
            if (remaining < COPY_HEADER_SIZE) {
                return FTOTA_ERR_TRUNCATED;
            }
            uint32_t base_off = read_u32_le(seg + seg_pos + 1);
            uint32_t len      = read_u32_le(seg + seg_pos + 5);
            memcpy(out + out_pos, base + base_off, len);
            seg_pos += COPY_HEADER_SIZE;
            out_pos += len;
        } else if (op == OP_ADD) {
            if (remaining < ADD_HEADER_SIZE) {
                return FTOTA_ERR_TRUNCATED;
            }
            uint32_t base_off = read_u32_le(seg + seg_pos + 1);
            uint32_t len      = read_u32_le(seg + seg_pos + 5);
            const uint8_t *diff = seg + seg_pos + ADD_HEADER_SIZE;
            for (uint32_t i = 0; i < len; i++) {
                out[out_pos + i] = (uint8_t)(base[base_off + i] + diff[i]);
            }
            seg_pos += ADD_HEADER_SIZE + len;
            out_pos += len;
        } else if (op == OP_LIT) {
            if (remaining < LIT_HEADER_SIZE) {
                return FTOTA_ERR_TRUNCATED;
            }
            uint32_t len = read_u32_le(seg + seg_pos + 1);
            if (len > remaining - LIT_HEADER_SIZE) {
                return FTOTA_ERR_TRUNCATED;
            }
            memcpy(out + out_pos, seg + seg_pos + LIT_HEADER_SIZE, len);
            seg_pos += LIT_HEADER_SIZE + len;
            out_pos += len;
        } else {
            return FTOTA_ERR_UNIMPLEMENTED;
        }
    }
    return FTOTA_OK;
}
