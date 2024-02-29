#ifndef MALORGITH_CLOGGER_HANDLER_H_
#define MALORGITH_CLOGGER_HANDLER_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_defines.h"
#include "logger_msg.h"

__MALORGITH_NAMESPACE_OPEN

#ifdef __cplusplus
typedef struct log_handler {
    int (*write)(struct log_handler*,const t_loggermsg*) = { NULL };
    int (*close)(struct log_handler*) = { NULL };
    int (*open)(struct log_handler*) = { NULL };
    int (*isOpen)(const struct log_handler*) = { NULL };
    bool allow_empty_line_;
    bool add_format_;
    void *handler_data_;
    log_handler(
        int (*)(struct log_handler*, const t_loggermsg*),
        int (*)(struct log_handler*),
        int (*)(struct log_handler*),
        int (*)(const struct log_handler*),
        bool allow_empty_line,
        bool add_format,
        void *handler_data
    );
    log_handler();
} log_handler;
#else
typedef struct log_handler {
    int (*write)(struct log_handler*,const t_loggermsg*);
    int (*close)(struct log_handler*);
    int (*open)(struct log_handler*);
    int (*isOpen)(const struct log_handler*);
    bool allow_empty_line_;
    bool add_format_;
    void *handler_data_;
} log_handler;
#endif  // __cplusplus

int lgh_init();
int lgh_free();

loghandler_t lgh_add_handler(log_handler const* handler);

/*
 * Used to check the sanity of data given to a handler function.
 */
int lgh_checks(
    const log_handler *handler,
    size_t handler_data_size,
    char const* caller_id
);

/*!
 * @brief Get the number of handlers that have been created.
 *
 * @returns the number of handlers that have been created, or < 0 on failure
 */
int lgh_get_num_handlers();

loghandler_t lgh_get_valid_handlers();
int lgh_remove_handler(loghandler_t handler_ref);
int lgh_remove_all_handlers();

/*!
 * @brief Write the message to the handlers referenced.
 *
 * @returns 0 on success, the number of handlers that couldn't be written to on
 * failure
 */
int lgh_write(loghandler_t handler_ref, t_loggermsg const* msg);
int lgh_write_to_all(t_loggermsg const* msg);

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_HANDLER_H_
