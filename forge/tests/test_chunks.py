import pytest
from hypothesis import given, strategies as st
from forge.chunks import chunk_count

def test_chunk_count_exact_multiple():
    assert chunk_count(target_size=65536, chunk_size=16384) == 4

def test_chunk_count_rounds_up_remainder():
    assert chunk_count(target_size=65537, chunk_size=16384) == 5

def test_chunk_count_rejects_unaligned_chunk_size():
    with pytest.raises(ValueError):
        chunk_count(target_size=1000, chunk_size=5000)

@given(size=st.integers(1, 4 << 20), k=st.integers(1, 16))
def test_chunks_cover_target_without_excess(size, k):
    cs = 4096 * k
    n = chunk_count(size, cs)
    assert (n - 1) * cs < size <= n * cs
