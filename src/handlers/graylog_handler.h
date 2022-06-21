
#ifndef GRAYLOG_HANDLER_H_INCLUDED
#define GRAYLOG_HANDLER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../logger_handler.h"

int create_graylog_handler(log_handler *p_pHandler, const char* p_sServer, int p_nPort, int p_nProtocol);

#ifdef __cplusplus
}
#endif

#endif
