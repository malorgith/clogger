#ifndef CONSOLE_HANDLER_H_INCLUDED
#define CONSOLE_HANDLER_H_INCLUDED

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_defines.h"
#include "logger_handler.h"

__MALORGITH_NAMESPACE_OPEN

int create_console_handler(log_handler *handler_ptr, FILE *file_out);

__MALORGITH_NAMESPACE_CLOSE

#endif
