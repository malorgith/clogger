#ifndef FILE_HANDLER_H_INCLUDED
#define FILE_HANDLER_H_INCLUDED

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_defines.h"
#include "logger_handler.h"

__MALORGITH_NAMESPACE_OPEN

int create_file_handler(log_handler *p_pHandler, char const* log_location_str, char const* log_name_str);

__MALORGITH_NAMESPACE_CLOSE

#endif  // FILE_HANDLER_H_INCLUDED
