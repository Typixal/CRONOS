ERASE_UNIT = 4096

def chunk_count(target_size: int, chunk_size: int) -> int:
    if chunk_size <= 0 or chunk_size % ERASE_UNIT:
        raise ValueError("chunk_size must be a positive multiple of 4096")
    return -(-target_size // chunk_size)
