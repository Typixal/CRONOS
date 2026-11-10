#include "ftota_patch.h"

#include <string.h>

#define OP_LIT 0x02

/* Minimal GREEN for the LIT-only test: one LIT op, u32 LE length, copy bytes.
 * No bounds checks yet. They arrive test-first (T03, T04). */
ftota_status_t ftota_apply_segment(const uint8_t *base, size_t base_len,
                                   const uint8_t *seg, size_t seg_len,
                                   uint8_t *out, size_t out_len)
{
    (void)base; (void)base_len; (void)seg_len; (void)out_len;

    if (seg[0] != OP_LIT) {
        return FTOTA_ERR_UNIMPLEMENTED;
    }
    uint32_t len = (uint32_t)seg[1]
                 | ((uint32_t)seg[2] << 8)
                 | ((uint32_t)seg[3] << 16)
                 | ((uint32_t)seg[4] << 24);
    memcpy(out, seg + 5, len);
    return FTOTA_OK;
}
