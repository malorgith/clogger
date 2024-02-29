#ifndef MALORGITH_CLOGGER_GRAYLOG_HANDLER_H_
#define MALORGITH_CLOGGER_GRAYLOG_HANDLER_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_defines.h"
#include "logger_handler.h"

__MALORGITH_NAMESPACE_OPEN

int create_graylog_handler(log_handler *handler, char const* graylog_url, int port, int protocol);

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_GRAYLOG_HANDLER_H_
