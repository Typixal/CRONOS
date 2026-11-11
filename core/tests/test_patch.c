#include "unity.h"
#include "ftota_patch.h"

void setUp(void) {}
void tearDown(void) {}

/* LIT op: 0x02, u32 LE length, literal bytes. Needs no base. */
void test_lit_only_segment_emits_literal_bytes(void) {
    const uint8_t seg[] = {0x02, 0x03, 0x00, 0x00, 0x00, 'a', 'b', 'c'};
    uint8_t out[3] = {0};

    ftota_status_t st = ftota_apply_segment(NULL, 0, seg, sizeof seg,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_OK, st);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("abc", out, 3);
}

/* COPY op: 0x00, u32 LE base_off, u32 LE len. Bytes come from base. */
void test_copy_segment_emits_bytes_from_base(void) {
    const uint8_t base[] = "hello world";
    const uint8_t seg[] = {0x00,
                           0x06, 0x00, 0x00, 0x00,   /* base_off = 6 */
                           0x05, 0x00, 0x00, 0x00};  /* len = 5 */
    uint8_t out[5] = {0};

    ftota_status_t st = ftota_apply_segment(base, sizeof base - 1,
                                            seg, sizeof seg,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_OK, st);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("world", out, 5);
}

/* Ops run in order, each appends to out. COPY then LIT. */
void test_copy_then_lit_ops_append_in_order(void) {
    const uint8_t base[] = "hello world";
    const uint8_t seg[] = {0x00,
                           0x00, 0x00, 0x00, 0x00,   /* base_off = 0 */
                           0x05, 0x00, 0x00, 0x00,   /* len = 5 */
                           0x02,
                           0x01, 0x00, 0x00, 0x00,   /* len = 1 */
                           '!'};
    uint8_t out[6] = {0};

    ftota_status_t st = ftota_apply_segment(base, sizeof base - 1,
                                            seg, sizeof seg,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_OK, st);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("hello!", out, 6);
}

/* ADD op: 0x01, u32 LE base_off, u32 LE len, then len diff bytes.
 * out[i] = base[base_off + i] + diff[i], wrapping mod 256. */
void test_add_segment_adds_diff_to_base_with_wraparound(void) {
    const uint8_t base[] = {0x00, 10, 20, 250};
    const uint8_t seg[] = {0x01,
                           0x01, 0x00, 0x00, 0x00,   /* base_off = 1 */
                           0x03, 0x00, 0x00, 0x00,   /* len = 3 */
                           1, 2, 10};                /* diff bytes */
    const uint8_t expected[] = {11, 22, 4};          /* 250 + 10 wraps to 4 */
    uint8_t out[3] = {0};

    ftota_status_t st = ftota_apply_segment(base, sizeof base,
                                            seg, sizeof seg,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_OK, st);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out, 3);
}

/* T03: LIT says 5 data bytes, but patch ends after 2. Must reject.
 * buf holds all 5 bytes on purpose, so the unfixed decoder reads valid
 * memory and fails by assertion, not by an ASan crash. */
void test_lit_with_truncated_data_is_rejected(void) {
    const uint8_t buf[] = {0x02,
                           0x05, 0x00, 0x00, 0x00,   /* len = 5 */
                           'a', 'b', 'c', 'd', 'e'};
    uint8_t out[5] = {0};

    /* seg_len = 7: op byte + 4 length bytes + only 2 of 5 data bytes */
    ftota_status_t st = ftota_apply_segment(NULL, 0, buf, 7,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_TRUNCATED, st);
}

/* T03: patch ends inside the LIT header. Only 2 of 4 length bytes exist.
 * buf is full size on purpose (same trick as above): unfixed decoder reads
 * valid memory, so RED is an assertion failure, not an ASan crash. */
void test_lit_with_truncated_header_is_rejected(void) {
    const uint8_t buf[] = {0x02,
                           0x05, 0x00, 0x00, 0x00,   /* len = 5 */
                           'a', 'b', 'c', 'd', 'e'};
    uint8_t out[5] = {0};

    /* seg_len = 3: op byte + only 2 of 4 length bytes */
    ftota_status_t st = ftota_apply_segment(NULL, 0, buf, 3,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_TRUNCATED, st);
}

/* T03: patch ends inside the COPY header. op + base_off + 1 of 4 len bytes.
 * buf is full size on purpose: unfixed decoder reads valid memory, so RED
 * is an assertion failure, not an ASan crash. */
void test_copy_with_truncated_header_is_rejected(void) {
    const uint8_t base[] = "hello world";
    const uint8_t buf[] = {0x00,
                           0x00, 0x00, 0x00, 0x00,   /* base_off = 0 */
                           0x05, 0x00, 0x00, 0x00};  /* len = 5 */
    uint8_t out[5] = {0};

    /* seg_len = 6: op byte + 4 base_off bytes + only 1 of 4 len bytes */
    ftota_status_t st = ftota_apply_segment(base, sizeof base - 1, buf, 6,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_TRUNCATED, st);
}

/* T03: patch ends inside the ADD header. op + base_off + 1 of 4 len bytes.
 * buf is full size and base has room for the reads, so unfixed decoder
 * touches only valid memory: RED is an assertion failure, not an ASan crash. */
void test_add_with_truncated_header_is_rejected(void) {
    const uint8_t base[] = {0x00, 10, 20, 250};
    const uint8_t buf[] = {0x01,
                           0x01, 0x00, 0x00, 0x00,   /* base_off = 1 */
                           0x03, 0x00, 0x00, 0x00,   /* len = 3 */
                           1, 2, 10};                /* diff bytes */
    uint8_t out[3] = {0};

    /* seg_len = 6: op byte + 4 base_off bytes + only 1 of 4 len bytes */
    ftota_status_t st = ftota_apply_segment(base, sizeof base, buf, 6,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_TRUNCATED, st);
}

/* T03: ADD header is complete (len = 3) but the patch holds only 1 of the
 * 3 diff bytes. buf is full size, so the unfixed decoder reads valid memory:
 * RED is an assertion failure, not an ASan crash. */
void test_add_with_truncated_diff_is_rejected(void) {
    const uint8_t base[] = {0x00, 10, 20, 250};
    const uint8_t buf[] = {0x01,
                           0x01, 0x00, 0x00, 0x00,   /* base_off = 1 */
                           0x03, 0x00, 0x00, 0x00,   /* len = 3 */
                           1, 2, 10};                /* diff bytes */
    uint8_t out[3] = {0};

    /* seg_len = 10: 9 header bytes + only 1 of 3 diff bytes */
    ftota_status_t st = ftota_apply_segment(base, sizeof base, buf, 10,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_TRUNCATED, st);
}

/* T04: COPY reads past end of base. base is 4 bytes, COPY wants 5 from
 * offset 2. out is big enough, so unfixed decoder fails by assertion
 * only if it reads valid memory: base2 is padded to hold the bad read. */
void test_copy_beyond_base_is_rejected(void) {
    const uint8_t base[16] = {0};          /* real memory is 16 bytes ... */
    const uint8_t seg[] = {0x00,
                           0x02, 0x00, 0x00, 0x00,   /* base_off = 2 */
                           0x05, 0x00, 0x00, 0x00};  /* len = 5 */
    uint8_t out[5] = {0};

    /* ... but we tell the decoder base_len = 4. 2 + 5 = 7 > 4 */
    ftota_status_t st = ftota_apply_segment(base, 4, seg, sizeof seg,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_RANGE, st);
}

/* T04: same for ADD. 3 diff bytes present, so truncation check passes. */
void test_add_beyond_base_is_rejected(void) {
    const uint8_t base[16] = {0};
    const uint8_t seg[] = {0x01,
                           0x02, 0x00, 0x00, 0x00,   /* base_off = 2 */
                           0x03, 0x00, 0x00, 0x00,   /* len = 3 */
                           1, 2, 3};
    uint8_t out[3] = {0};

    ftota_status_t st = ftota_apply_segment(base, 4, seg, sizeof seg,
                                            out, sizeof out);

    TEST_ASSERT_EQUAL_INT(FTOTA_ERR_RANGE, st);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lit_only_segment_emits_literal_bytes);
    RUN_TEST(test_copy_segment_emits_bytes_from_base);
    RUN_TEST(test_copy_then_lit_ops_append_in_order);
    RUN_TEST(test_add_segment_adds_diff_to_base_with_wraparound);
    RUN_TEST(test_lit_with_truncated_data_is_rejected);
    RUN_TEST(test_lit_with_truncated_header_is_rejected);
    RUN_TEST(test_copy_with_truncated_header_is_rejected);
    RUN_TEST(test_add_with_truncated_header_is_rejected);
    RUN_TEST(test_add_with_truncated_diff_is_rejected);
    RUN_TEST(test_copy_beyond_base_is_rejected);
    RUN_TEST(test_add_beyond_base_is_rejected);
    return UNITY_END();
}
