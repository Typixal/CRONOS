#ifndef FTOTA_PATCH_H
#define FTOTA_PATCH_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    FTOTA_OK = 0,
    FTOTA_ERR_UNIMPLEMENTED = -1,
    FTOTA_ERR_TRUNCATED = -2,     /* patch ends before an op is complete */
    FTOTA_ERR_RANGE = -3
} ftota_status_t;

/* Decode one patch segment into out[0..out_len). Base is read-only. */
ftota_status_t ftota_apply_segment(const uint8_t *base, size_t base_len,
                                   const uint8_t *seg, size_t seg_len,
                                   uint8_t *out, size_t out_len);

#endif
