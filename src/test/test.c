
#include <stdbool.h>

#include <CUnit/Basic.h>

#include "clogger.h"

#include "test/clogger_test.h"

typedef struct {
    CU_pSuite suite_;
    int (*ptr_)(CU_pSuite);
} TestStruct;

int main(
    __attribute__((unused))int argc,
    __attribute__((unused))char** argv
) {

    // init CUnit test registry
    if (CU_initialize_registry() != CUE_SUCCESS) {
        fprintf(stderr, "Failed to initialize CUnit.\n");
        return 1;
    }

    TestStruct test_structs[] = {
        {NULL, &init_buffer_suite},
        {NULL, &init_format_suite},
        {NULL, &init_handler_suite},
        {NULL, &init_id_suite},
        {NULL, &init_level_suite},
        {NULL, &init_logger_suite},
        {NULL, &init_msg_suite}
    };

    int num_tests = sizeof(test_structs) / sizeof(TestStruct);
    for (int count = 0; count < num_tests; count++) {
        int init_rtn = (*test_structs[count].ptr_)(test_structs[count].suite_);
        if (init_rtn) {
            return init_rtn;
        }
    }

    // run all tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
