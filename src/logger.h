
#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*! \file logger.h
 *
 * A threaded logger that places an emphasis on minimizing the
 * time spent sending a message.
 * 
 */

#include "logger_handler.h"
#include "logger_id.h"

#include <stdarg.h>

#define LOGGER_MAX_NUM_FORMATTERS   5

#ifdef TESTING_FUNCTIONS_ENABLED
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
void *stress_test_logger(void *arg);
bool logger_run_stress_test(int p_nThreads, int p_nMessages);
#endif

#endif

#ifdef __cplusplus
}
#endif
