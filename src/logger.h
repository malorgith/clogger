#ifndef MALORGITH_CLOGGER_LOGGER_H_
#define MALORGITH_CLOGGER_LOGGER_H_

#include "logger_defines.h"

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_handler.h"
#include "logger_id.h"

#include <stdarg.h>

#define LOGGER_MAX_NUM_FORMATTERS   5

__MALORGITH_NAMESPACE_OPEN

/*! this function must be exposed for c++ */
int _logger_log_msg(
    int log_level,
    logid_t id,
    char* format,
    char const* msg,
    va_list arg_list
);

#ifndef NDEBUG
int logger_print_id(logid_t id_ref);

/*!
 * Returns the value of g_nBufferReadIndex.
 *
 * This function is intended to be used with test functions.
 */
int logger_get_read_index();

/*!
 * Returns the value of g_nBufferAddIndex.
 *
 * This function is intended to be used with test functions.
 */
int logger_get_add_index();

bool logger_test_stop_logger();
bool custom_test_logger_large_add_message();
#endif  // NDEBUG

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_LOGGER_H_
