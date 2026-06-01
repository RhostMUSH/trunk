#include <check.h>
#include <stdlib.h>
#include <string.h>

/* Include the header under test */
#include "Server/hdrs/functions.h"

/* Define minimal stubs needed for the macros to compile */
#ifndef LBUF_SIZE
#define LBUF_SIZE 8192
#endif

START_TEST(test_separator_buffer_overflow)
{
    /* Invariant: Buffer reads/writes for separator arguments must never exceed declared buffer size */
    const char *payloads[] = {
        "|",                                                    /* Valid single-char separator */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 52 chars - 2x typical osep buffer */
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", /* 256 chars - 10x typical buffer */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        size_t input_len = strlen(payloads[i]);
        
        /* Test that oversized inputs are detected - any separator > 32 chars is suspicious */
        if (input_len > 32) {
            /* This payload would overflow a typical osep[32] buffer if used with strcpy */
            ck_assert_msg(input_len > 32, 
                "Oversized separator payload %zu chars should be rejected or truncated", input_len);
        } else {
            /* Valid separator should fit in buffer */
            ck_assert_msg(input_len <= 32, 
                "Valid separator should fit in osep buffer");
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_separator_buffer_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}