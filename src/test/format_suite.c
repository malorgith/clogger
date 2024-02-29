
#include "test/clogger_test.h"

#include <stdbool.h>

#include <CUnit/CUnit.h>

#include "clogger.h"

#include "logger_defines.h"

static void test_max_size(void) {
    // check max format length
    CU_ASSERT(kCloggerMaxFormatSize >= 30);
}

int init_format_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger Format", NULL, NULL);
    if (!suite) {
        fprintf(stderr, "Failed to add format suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (
        (!CU_add_test(suite, "check_max_size", test_max_size))
    ) {
        fprintf(stderr, "Failed to add test to the format suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
