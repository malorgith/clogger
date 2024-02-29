
#include "test/clogger_test.h"

#include <CUnit/CUnit.h>

#include "clogger.h"

#include "logger_defines.h"
#include "test/common_util.h"

static void test_logger_set_log_level(void) {
    // try setting a negative level
    CU_ASSERT(logger_set_log_level(-1) != 0);
    // try setting a level that's too large
    CU_ASSERT(logger_set_log_level(LOGGER_MAX_LEVEL + 1) != 0);
    // try setting a valid level
    CU_ASSERT(logger_set_log_level(kCloggerDebug) == 0);
}

static void test_log_level_sanity(void) {
    // verify that all log levels are sane
    CU_ASSERT(kCloggerEmergency < kCloggerAlert);
    CU_ASSERT(kCloggerAlert < kCloggerCritical);
    CU_ASSERT(kCloggerCritical < kCloggerError);
    CU_ASSERT(kCloggerError < kCloggerWarn);
    CU_ASSERT(kCloggerWarn < kCloggerNotice);
    CU_ASSERT(kCloggerNotice < kCloggerInfo);
    CU_ASSERT(kCloggerInfo < kCloggerDebug);
}

static void test_get_log_level(void) {
    int log_level = logger_get_log_level();
    CU_ASSERT(log_level <= kCloggerDebug);
    CU_ASSERT(log_level >= kCloggerEmergency);
}

static void test_log_str_to_int(void) {
    // TODO(malorgith): these should be pulled from somewhere or exported in public header
    char const* debug_str = "debug";
    char const* info_str = "info";
    char const* emergency_str = "emergency";
    char const* bad_str = "not_valid_level";
    CU_ASSERT(logger_log_str_to_int(debug_str) == kCloggerDebug);
    CU_ASSERT(logger_log_str_to_int(info_str) == kCloggerInfo);
    CU_ASSERT(logger_log_str_to_int(emergency_str) == kCloggerEmergency);
    int bad_level = logger_log_str_to_int(bad_str);
    CU_ASSERT((bad_level < kCloggerEmergency) || (bad_level > kCloggerDebug));
}

int init_level_suite(CU_pSuite suite) {
    suite = CU_add_suite("clogger Level", generic_init_suite, generic_clean_suite);
    if (!suite) {
        fprintf(stderr, "Failed to add level suite to registry.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (
        !CU_add_test(suite, "logger_set_log_level", test_logger_set_log_level) ||
        !CU_add_test(suite, "logger_get_log_level", test_get_log_level) ||
        !CU_add_test(suite, "logger_log_str_to_int", test_log_str_to_int) ||
        !CU_add_test(suite, "check_log_level_sanity", test_log_level_sanity)
    ) {
        fprintf(stderr, "Failed to add test to the level suite.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
