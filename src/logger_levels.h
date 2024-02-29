#ifndef MALORGITH_CLOGGER_LEVELS_H_
#define MALORGITH_CLOGGER_LEVELS_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

//#include "clogger.h"
#include "logger_defines.h"
#include "logger_util.h"
#include __MALORGITH_CLOGGER_INCLUDE

__MALORGITH_NAMESPACE_OPEN

extern char const* lgl_lchars[];
extern char const* lgl_uchars[];
extern char const* lgl_lstrs[];
extern char const* lgl_ustrs[];

/*!
 * @brief Check if the given log level is valid.
 *
 * @returns 0 if the level is valid, non-zero if it isn't
 */
int lgl_check(
    /*! the log level to check */
    int log_level
);

/*!
 * @brief Find the maximum length from a given array of log level strings.
 *
 * @returns the length of the longest string, < 0 if there's an error
 */
int lgl_get_max_len(
    /*! an array of log level strings to iterate over */
    char const** lvl_strs
);

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_LEVELS_H_
