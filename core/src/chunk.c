#include "ftota_chunk.h"

#define FTOTA_ERASE_UNIT 4096u

bool ftota_chunk_size_valid(uint32_t chunk_size) {
    return chunk_size != 0 && (chunk_size % FTOTA_ERASE_UNIT) == 0;
}
