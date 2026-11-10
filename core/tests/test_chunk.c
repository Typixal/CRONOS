#include "unity.h"
#include "ftota_chunk.h"

void setUp(void) {}
void tearDown(void) {}

/* NFR4: chunk size multiple of 4096 (flash erase unit). */
void test_chunk_size_accepts_4k_multiples(void) {
    TEST_ASSERT_TRUE(ftota_chunk_size_valid(4096));
    TEST_ASSERT_TRUE(ftota_chunk_size_valid(16384));
    TEST_ASSERT_TRUE(ftota_chunk_size_valid(65536));
}
void test_chunk_size_rejects_zero(void) { TEST_ASSERT_FALSE(ftota_chunk_size_valid(0)); }
void test_chunk_size_rejects_non_multiple(void) {
    TEST_ASSERT_FALSE(ftota_chunk_size_valid(4095));
    TEST_ASSERT_FALSE(ftota_chunk_size_valid(6000));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chunk_size_accepts_4k_multiples);
    RUN_TEST(test_chunk_size_rejects_zero);
    RUN_TEST(test_chunk_size_rejects_non_multiple);
    return UNITY_END();
}
