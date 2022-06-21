
#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_handler.h"
#include "logger_id.h"

#include <stdarg.h>

#define LOGGER_MAX_NUM_FORMATTERS   5

#ifndef NDEBUG
int logger_print_id(logger_id id_ref);

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
#endif

#endif

#ifdef __cplusplus
}
#endif
