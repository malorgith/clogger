#ifndef MALORGITH_CLOGGER_MSG_H_
#define MALORGITH_CLOGGER_MSG_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include "logger_levels.h"

#define LOGGER_MAX_FORMAT_SIZE 100

#include <time.h>

#include "logger_defines.h"

__MALORGITH_NAMESPACE_OPEN

typedef struct {
    /*! the fully formatted message */
    char msg_[CLOGGER_MAX_MESSAGE_SIZE];
    /*! the log id string for the message */
    char id_[CLOGGER_ID_MAX_LEN];
    /*! the log level of the message */
    int log_level_;
    /*! the reference to the logid_t for the message */
    logid_t ref_id_;
    /*! the format string that should be used for the message */
    char* format_;
    /*! the timestamp for the message */
    time_t timestamp_;
} t_loggermsg;

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_MSG_H_
