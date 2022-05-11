
#ifndef LOGGER_MSG_H_INCLUDED
#define LOGGER_MSG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "logger_levels.h"
#include "logger_id.h"

#define LOGGER_MAX_FORMAT_SIZE 100

#include <time.h>

typedef struct {
    char        m_sMsg[LOGGER_MAX_MESSAGE_SIZE];
    int         m_nLogLevel;
    logger_id   m_nId;
    char*       m_sFormat;
    char        m_sId[LOGGER_ID_MAX_LEN];
    struct tm   m_tmTime;
} t_loggermsg;

#ifdef __cplusplus
}
#endif

#endif
