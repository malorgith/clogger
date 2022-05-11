
#ifndef HANDLER_H_INCLUDED
#define HANDLER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_msg.h"

typedef struct {
    int (*write)(t_loggermsg*);
    int (*close)();
    int (*open)();
    int (*isOpen)();
    bool m_bAllowEmptyLine;
    bool m_bAddFormat;
} log_handler;

#ifdef __cplusplus
}
#endif

#endif
