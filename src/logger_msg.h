
#ifndef LOGGER_MSG_H_INCLUDED
#define LOGGER_MSG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_levels.h"

#define LOGGER_MAX_FORMAT_SIZE 100

#include <time.h>

typedef struct {
    char            m_sMsg[CLOGGER_MAX_MESSAGE_SIZE];
    char            m_sId[CLOGGER_ID_MAX_LEN];
    int             m_nLogLevel;
    logger_id       m_refId;
    char*           m_sFormat;
    struct tm       m_tmTime;
} t_loggermsg;

#ifdef __cplusplus
}
#endif

#endif
