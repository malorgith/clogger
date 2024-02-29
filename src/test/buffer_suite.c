
#include "test/clogger_test.h"

#include <CUnit/CUnit.h>

#include "clogger.h"

static void test_max_size(void) {
    // check max buffer length
    CU_ASSERT(kCloggerBufferSize >= 10);
}

int init_buffer_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger Buffer", NULL, NULL);
    if (!suite) {
        fprintf(stderr, "Failed to add buffer suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (
        !CU_add_test(suite, "check_max_size", test_max_size)
    ) {
        fprintf(stderr, "Failed to add test to the buffer suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
