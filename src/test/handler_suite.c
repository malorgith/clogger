
#include "test/clogger_test.h"

#include <CUnit/CUnit.h>

#include "clogger.h"

static void test_max_num_handlers(void) {
    CU_ASSERT(kCloggerMaxNumHandlers >= 2);  // check max handlers
}

int init_handler_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger Handler", NULL, NULL);
    if (!suite) {
        fprintf(stderr, "Failed to add handler suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (
        (!CU_add_test(suite, "check_max_num", test_max_num_handlers))
    ) {
        fprintf(stderr, "Failed to add test to the handler suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
