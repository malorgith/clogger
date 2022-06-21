
#ifndef LOGGER_BUFFER_H_INCLUDED
#define LOGGER_BUFFER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_msg.h"

int lgb_init();

int lgb_free();

int lgb_create_buffer();

int lgb_remove_buffer(int bufref);

int lgb_add_message(int bufref, t_loggermsg* msg);

t_loggermsg* lgb_read_message(int bufref);

int lgb_wait_for_messages(int bufref, int seconds_to_wait);

#ifndef NDEBUG
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
