
#include "test/clogger_test.h"

#include <stdbool.h>

#include <CUnit/CUnit.h>

#include "clogger.h"

#include "test/common_util.h"

static void test_max_message_size(void) {
    // check message length
    CU_ASSERT(kCloggerMaxMessageSize >= 10);

    // log a message that's too long
    {
        int pad_size = 5;
        int num_msg_chars = kCloggerMaxMessageSize + pad_size;
        char large_msg[num_msg_chars];
        for (int count = 0; count < num_msg_chars - 2; count++) {
            large_msg[count] = ' ';
        }
        large_msg[(num_msg_chars) - 2] = 'f';
        large_msg[(num_msg_chars) - 1] = '\0';
        CU_ASSERT(logger_log_msg(kCloggerInfo, "%s", large_msg) != 0);
    }
}

int init_msg_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger Message", generic_init_suite, generic_clean_suite);
    if (!suite) {
        fprintf(stderr, "Failed to add message suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (!CU_add_test(suite, "check_max_size", test_max_message_size)) {
        fprintf(stderr, "Failed to add test to the message suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
