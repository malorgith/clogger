#ifndef MALORGITH_CLOGGER_UTIL_H_
#define MALORGITH_CLOGGER_UTIL_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

#include "logger_defines.h"

__MALORGITH_NAMESPACE_OPEN

/*!
 * @brief Add the specified milliseconds to the timespec.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgu_add_to_time(
    /*! the timespec to modify */
    struct timespec *ptr_timespec,
    /*! the number of milliseconds to add */
    int ms_to_add,
    /*! the minimum number of milliseconds that can be added */
    int min_ms,
    /*! the maximum number of milliseconds that can be added */
    int max_ms
);

/*!
 * @brief Check if the given file can be written to.
 *
 * @returns 0 if the file can be written, non-zero if it can't be written
 */
int lgu_can_write(
    /*! the path to the file to check */
    char const* str_file_path
);

/*!
 * @brief Check if the given path is a directory.
 *
 * @returns zero if the path is a directory, non-zero if it isn't a directory
 */
int lgu_is_dir(
    /*! the path to the directory */
    char const* str_dir_path
);

/*!
 * @brief Waits for the semaphore for the specified time.
 *
 * @returns 0 if the semaphore is available before the time ends, a positive
 * value if the wait timed out, or a negative value if an unspecified error
 * occurred.
 */
int lgu_timedwait(
    /*! pointer to a semaphore to wait for */
    sem_t *sem_ptr,
    /*! number of milliseconds to wait */
    int milliseconds_to_wait
);

/*!
 * @brief Utility function for snprintf.
 *
 * @returns zero if the string was stored, non-zero if it wasn't
 */
int lgu_wsnprintf(
    /*! the location to place the formatted string */
    char* dest,
    /*! the maximum amount that can be written to the string */
    int max_size,
    /*! the warning message to print if formatting fails */
    char const* str_warn_msg,
    /*! the format of the string to store in dest */
    char const* format,
    /*! arguments to modify the format given */
    ...
);

/*!
 * @brief Print a warning message.
 */
void lgu_warn_msg(
    /*! the message to print */
    char const* msg
);

/*!
 * @brief Print a warning message that contains an integer to format.
 */
void lgu_warn_msg_int(
    /*! format string that expects one integer */
    char const* format,
    /*! integer to place in format string */
    const int input_int
);

/*!
 * @brief Print a warning message that contains a string to format.
 */
void lgu_warn_msg_str(
    /*! format string that expects one string */
    char const* format,
    /*! string to place in format string */
    char const* intput_str
);

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_UTIL_H_
