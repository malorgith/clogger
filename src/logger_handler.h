
#ifndef HANDLER_H_INCLUDED
#define HANDLER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_msg.h"

typedef struct {
    int (*const write)(const t_loggermsg*);
    int (*const close)();
    int (*const open)();
    int (*const isOpen)();
    bool m_bAllowEmptyLine;
    bool m_bAddFormat;
} log_handler;

typedef uint8_t t_handlerref;

int lgh_init();
int lgh_free();

int lgh_add_handler(const log_handler* p_pHandler);
int lgh_get_num_handlers();
int lgh_open_handlers();
int lgh_remove_handler(t_handlerref p_refIndex);
int lgh_remove_all_handlers();
int lgh_write(t_handlerref p_nHandlerRef, const t_loggermsg *p_pMsg);
int lgh_write_to_all(const t_loggermsg *p_pMsg);

#ifdef __cplusplus
}
#endif

#endif
