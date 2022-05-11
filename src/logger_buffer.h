
#ifndef LOGGER_BUFFER_H_INCLUDED
#define LOGGER_BUFFER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_msg.h"

int logger_buffer_init();

int logger_buffer_free();

int logger_buffer_create_buffer();

int logger_buffer_free_buffer(int bufref);

int logger_buffer_add_message(int bufref, t_loggermsg* msg);

t_loggermsg* logger_buffer_read_message(int bufref);

int logger_buffer_wait_for_messages(int bufref, int seconds_to_wait);

int logger_buffer_add_to_time(struct timespec *p_pTspec, int ms_to_add, int min_ms, int max_ms);

#ifdef TESTING_FUNCTIONS_ENABLED
/*!
 * Returns the value of BUFFER_SIZE.
 *
 * This function is intended to be used with test functions.
 */
int logger_get_buffer_size();

int logger_get_buffer_close_warn();
#endif

#ifdef __cplusplus
}
#endif

#endif
