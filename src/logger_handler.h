
#ifndef HANDLER_H_INCLUDED
#define HANDLER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_msg.h"

typedef struct log_handler {
    int (*const write)(struct log_handler*,const t_loggermsg*);
    int (*const close)(struct log_handler*);
    int (*const open)(struct log_handler*);
    int (*const isOpen)(const struct log_handler*);
    bool m_bAllowEmptyLine;
    bool m_bAddFormat;
    void *m_pHandlerData;
} log_handler;

int lgh_init();
int lgh_free();

t_handlerref lgh_add_handler(const log_handler* p_pHandler);

/*
 * Used to check the sanity of data given to a handler function.
 */
int lgh_checks(const log_handler *p_pHandler, size_t p_sizeData, const char* p_sCaller);

int lgh_get_num_handlers();
t_handlerref lgh_get_valid_handlers();
int lgh_remove_handler(t_handlerref p_refIndex);
int lgh_remove_all_handlers();
int lgh_write(t_handlerref p_nHandlerRef, const t_loggermsg *p_pMsg);
int lgh_write_to_all(const t_loggermsg *p_pMsg);

#ifdef __cplusplus
}
#endif

#endif
