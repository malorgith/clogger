
#include "logger_levels.h"

#include <string.h>

// global vars

/*
 * Note that the 'chars' are actually strings. This is to make it so
 * '%s' can consistently be used in formatters to output the value.
 */
const char* lgl_lchars[] = {
    "!",
    "a",
    "c",
    "e",
    "w",
    "n",
    "i",
    "d"
};

const char* lgl_uchars[] = {
    "!",
    "A",
    "C",
    "E",
    "W",
    "N",
    "I",
    "D"
};

const char* lgl_lstrs[] = {
    "emergency",
    "alert",
    "critical",
    "error",
    "warn",
    "notice",
    "info",
    "debug"
};

const char* lgl_ustrs[] = {
    "EMERGENCY",
    "ALERT",
    "CRITICAL",
    "ERROR",
    "WARN",
    "NOTICE",
    "INFO",
    "DEBUG"
};

// public functions
int lgl_check(int log_level) {
    if (log_level < 0) {
        lgu_warn_msg("log level is less than 0");
        return 1;
    }
    else if (log_level > LOGGER_MAX_LEVEL) {
        lgu_warn_msg_int("log level is greater than max level '%d'", LOGGER_MAX_LEVEL);
        return 1;
    }
    return 0;
}

int lgl_get_max_len(const char** lvl_strs) {
    if (lvl_strs == NULL)
        return -1;

    int longest_len = -1;
    for (int count = 0; count < LOGGER_MAX_LEVEL; count++) {
        int new_len = strlen(lvl_strs[count]);
        if (new_len > longest_len)
            longest_len = new_len;
    }

    return longest_len;
}

