
#include "test/common_util.h"

#include "clogger.h"

int generic_clean_suite(void) {
    return logger_free();
}

int generic_init_suite(void) {
    return logger_init(kCloggerDebug);
}
