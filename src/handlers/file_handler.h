
#ifndef FILE_HANDLER_H_INCLUDED
#define FILE_HANDLER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../logger_handler.h"

int create_file_handler(log_handler *p_pHandler, const char* p_sLogLocation, const char* p_sLogName);

#ifdef __cplusplus
}
#endif

#endif
