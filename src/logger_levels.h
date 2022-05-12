
#ifndef LOGGER_LEVELS_H_INCLUDED
#define LOGGER_LEVELS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "clogger.h"
#include "logger_util.h"

extern const char* lgl_lchars[];
extern const char* lgl_uchars[];
extern const char* lgl_lstrs[];
extern const char* lgl_ustrs[];

/*
 * Checks to make sure the log level passed is in a valid
 * range. Uses LOGGER_MAX_LEVEL to determine maximum value.
 */
int lgl_check(int log_level);

int lgl_get_max_len(const char** lvl_strs);

#ifdef __cplusplus
}
#endif

#endif
