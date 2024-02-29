
#include "test/clogger_test.h"

#include <CUnit/CUnit.h>

#include "clogger.h"

#include "test/common_util.h"

static void test_max_id_len(void) {
    // check max ID length
    CU_ASSERT(kCloggerIdMaxLen >= 5);

    // try to create an ID that's too long
    {
        int pad_size = 5;
        int num_id_chars = kCloggerIdMaxLen + pad_size;
        char large_id[num_id_chars];
        for (int count = 0; count < num_id_chars - 2; count++) {
            large_id[count] = ' ';
        }
        large_id[(num_id_chars) - 2] = 'f';
        large_id[(num_id_chars) - 1] = '\0';
        CU_ASSERT(logger_create_id(large_id) == kCloggerMaxNumIds);
    }
}

static void test_max_num_ids(void) {
    // check number of IDs
    CU_ASSERT(kCloggerMaxNumIds >= 3);

    // try to add too many IDs
    {
        // fill available IDs; default ID takes a spot
        logid_t id_refs[kCloggerMaxNumIds];
        id_refs[0] = kCloggerDefaultId;
        for (int count = 1; count < kCloggerMaxNumIds; count++) {
            int id_size = 15;
            char id_str[id_size];
            snprintf(id_str, id_size, "%03d", count);
            id_refs[count] = logger_create_id(id_str);
            CU_ASSERT(id_refs[count] != kCloggerMaxNumIds);
        }
        // try to add an additional ID
        CU_ASSERT(logger_create_id("final_id") == kCloggerMaxNumIds);

        // clean up the IDs that were created
        for (int count = (kCloggerMaxNumIds - 1); count > kCloggerDefaultId + 1; count--) {
            CU_ASSERT_FALSE(logger_remove_id(id_refs[count]));
        }
    }

    // TODO(malorgith): test creating ID with handlers
}

static void test_logger_create_id(void) {
    logid_t id_ref = logger_create_id("test_id");
    CU_ASSERT(id_ref != kCloggerMaxNumIds);
    CU_ASSERT_FALSE_FATAL(logger_remove_id(id_ref));
}

static void test_logger_remove_id(void) {
    logid_t id_ref = logger_create_id("test_id");
    CU_ASSERT_FATAL(id_ref != kCloggerMaxNumIds);
    CU_ASSERT_FALSE(logger_remove_id(id_ref));
}

int init_id_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger ID", generic_init_suite, generic_clean_suite);
    if (!suite) {
        fprintf(stderr, "Failed to add ID suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (
        !CU_add_test(suite, "logger_create_id", test_logger_create_id) ||
        !CU_add_test(suite, "logger_remove_id", test_logger_remove_id) ||
        !CU_add_test(suite, "check_max_id_len", test_max_id_len) ||
        !CU_add_test(suite, "check_max_num_ids", test_max_num_ids)
    ) {
        fprintf(stderr, "Failed to add test to the ID suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
