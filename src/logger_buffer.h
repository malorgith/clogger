#ifndef MALORGITH_CLOGGER_BUFFER_H_
#define MALORGITH_CLOGGER_BUFFER_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_defines.h"
#include "logger_msg.h"

__MALORGITH_NAMESPACE_OPEN

/*!
 * @brief Initialize the log buffers.
 *
 * @returns the buffer index on success, < 0 on failure
 */
int lgb_init();

/*!
 * @brief Free memory used by the buffers.
 *
 * @returns 0 on success, > 0 on failure
 */
int lgb_free();

/*!
 * @brief Create a new buffer.
 *
 * @returns the index of the new buffer, < 0 on failure
 */
int lgb_create_buffer();

/*!
 * @brief Delete a buffer.
 *
 * @returns 0 on success, > 0 on failure
 */
int lgb_remove_buffer(
    /*! the reference to the buffer to remove */
    int bufref
);

/*!
 * @brief Add a message to the referenced buffer.
 *
 * @returns 0 on success, > 0 on failure
 */
int lgb_add_message(
    /*! the reference to the buffer to add the message to */
    int bufref,
    /*! the message to add to the buffer */
    t_loggermsg* msg
);

/*!
 * @brief Get a message from the referenced buffer.
 *
 * @returns the next message on the buffer, NULL on failure
 */
t_loggermsg* lgb_read_message(
    /*! the reference to the buffer to read from */
    int bufref
);

/*!
 * @brief Wait for a message to arrive on the buffer.
 *
 * @returns 0 if a message is on the buffer, > 0 if milliseconds_to_wait
 * expires, or < 0 if an error occurred
 */
int lgb_wait_for_messages(
    /*! the reference to the buffer to wait for messages on */
    int bufref,
    /*! the number of milliseconds to wait */
    int milliseconds_to_wait
);

#ifndef NDEBUG
/*!
 * @brief Get the value of BUFFER_SIZE.
 *
 * This function is intended to be used with test functions.
 *
 * @returns the integer value of BUFFER_SIZE
 */
int logger_get_buffer_size();

/*!
 * @brief Get the value of BUFFER_CLOSE_WARN.
 *
 * This function is intended to be used with test functions.
 *
 * @returns the integer value of BUFFER_CLOSE_WARN
 */
int logger_get_buffer_close_warn();
#endif  // NDEBUG

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_BUFFER_H_
