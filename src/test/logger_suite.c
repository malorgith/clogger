
#include "test/clogger_test.h"

#include <CUnit/CUnit.h>

#include "clogger.h"

#include "logger_defines.h"

static void test_logger_init(void) {
    CU_ASSERT_TRUE(logger_init(-1));  // try a level that's too low
    CU_ASSERT_TRUE(logger_init(LOGGER_MAX_LEVEL + 1));  // try a level that's too high
    CU_ASSERT_FALSE(logger_init(kCloggerDebug));  // set a valid level
    CU_ASSERT_TRUE(logger_init(kCloggerDebug));  // try to start it again
    CU_ASSERT_FALSE_FATAL(logger_free());  // clean up
}

static void test_logger_free(void) {
    CU_ASSERT_TRUE(logger_free());  // try to free the logger without it running
    CU_ASSERT_FALSE_FATAL(logger_init(kCloggerDebug));  // start the logger
    CU_ASSERT_FALSE(logger_free());  // free the logger
    CU_ASSERT_TRUE(logger_free());  // try to free the logger again
}

static void test_logger_is_running(void) {
    CU_ASSERT_FALSE(logger_is_running());  // check status when it hasn't been started
    CU_ASSERT_FALSE_FATAL(logger_init(kCloggerDebug));  // start the logger
    CU_ASSERT_TRUE(logger_is_running());  // check status when it's running
    CU_ASSERT_FALSE_FATAL(logger_free());  // free the logger
    CU_ASSERT_FALSE(logger_is_running());  // check status after free
}

static void test_logger_log_msg(void) {
    CU_ASSERT_TRUE(logger_log_msg(kCloggerInfo, "message"));  // check when logger isn't running
    CU_ASSERT_FALSE_FATAL(logger_init(kCloggerInfo));  // start the logger
    CU_ASSERT_TRUE(logger_log_msg(-1, "message"));  // check negative log level
    // check valid message
    CU_ASSERT_FALSE(logger_log_msg(kCloggerInfo, "message"));
    // check message is valid when global level isn't met
    CU_ASSERT_FALSE(logger_log_msg(kCloggerDebug, "message"));
    CU_ASSERT_FALSE_FATAL(logger_free());  // free the logger
}

static void test_logger_log_msg_id(void) {
    // TODO (maybe?) test valid response when level too high; can we verify nothing was written?
    // check when logger isn't running
    CU_ASSERT_TRUE(logger_log_msg_id(kCloggerInfo, kCloggerDefaultId, "message"));
    CU_ASSERT_FALSE_FATAL(logger_init(kCloggerInfo));  // start the logger
    // check negative log level
    CU_ASSERT_TRUE(logger_log_msg_id(-1, kCloggerDefaultId, "message"));
    logid_t id_ref_1 = logger_create_id("test_id");  // create an id
    CU_ASSERT_FATAL(id_ref_1 != kCloggerMaxNumIds);  // check that the id is valid
    // check valid message
    CU_ASSERT_FALSE(logger_log_msg_id(kCloggerInfo, id_ref_1, "message"));
    logid_t id_ref_2 = logger_create_id("test_id");  // create an id
    CU_ASSERT_FATAL(id_ref_2 != kCloggerMaxNumIds);  // check that the id is valid
    // check message is valid when global level isn't met
    CU_ASSERT_FALSE(logger_log_msg_id(kCloggerDebug, id_ref_2, "message"));
    CU_ASSERT_FALSE_FATAL(logger_remove_id(id_ref_1));  // remove the id
    CU_ASSERT_FALSE_FATAL(logger_remove_id(id_ref_2));  // remove the id
    // check logging to an invalid id
    CU_ASSERT_TRUE(logger_log_msg_id(kCloggerInfo, id_ref_1, "message"));
    CU_ASSERT_FALSE_FATAL(logger_free());  // free the logger
}

int init_logger_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger Logger", NULL, NULL);
    if (!suite) {
        fprintf(stderr, "Failed to add logger suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (
        !CU_add_test(suite, "logger_init", test_logger_init) ||
        !CU_add_test(suite, "logger_free", test_logger_free) ||
        !CU_add_test(suite, "logger_log_msg", test_logger_log_msg) ||
        !CU_add_test(suite, "logger_log_msg_id", test_logger_log_msg_id) ||
        !CU_add_test(suite, "logger_is_running", test_logger_is_running)
    ) {
        fprintf(stderr, "Failed to add test to the logger suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
