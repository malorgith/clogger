
#ifndef CONSOLE_HANDLER_H_INCLUDED
#define CONSOLE_HANDLER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../logger_handler.h"

int create_console_handler(log_handler *p_pHandler, FILE *p_pOut);

#ifdef __cplusplus
}
#endif

#endif
