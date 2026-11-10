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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lit_only_segment_emits_literal_bytes);
    RUN_TEST(test_copy_segment_emits_bytes_from_base);
    return UNITY_END();
}
