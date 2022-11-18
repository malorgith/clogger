
#ifndef CLOGGER_H_INCLUDED
#define CLOGGER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

/*! \file clogger.h
 *
 * A threaded logger that places an emphasis on minimizing the
 * time spent by the calling thread when logging a message.
 *
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#if _POSIX_C_SOURCE < 200112L
#error "_POSIX_C_SOURCE must be at least 200112L"
#endif

// follows RFC 5424
#define LOGGER_EMERGENCY    0
#define LOGGER_ALERT        1
#define LOGGER_CRITICAL     2
#define LOGGER_ERROR        3
#define LOGGER_WARN         4
#define LOGGER_NOTICE       5
#define LOGGER_INFO         6
#define LOGGER_DEBUG        7

#define LOGGER_MAX_LEVEL    7

#ifndef CLOGGER_MAX_MESSAGE_SIZE
#define CLOGGER_MAX_MESSAGE_SIZE 200
#endif

#ifndef CLOGGER_ID_MAX_LEN
#define CLOGGER_ID_MAX_LEN   30
#endif

#ifndef CLOGGER_MAX_NUM_IDS
#define CLOGGER_MAX_NUM_IDS 20
#endif

#ifndef CLOGGER_MAX_NUM_HANDLERS
#define CLOGGER_MAX_NUM_HANDLERS 5
#endif

#ifndef CLOGGER_BUFFER_SIZE
#define CLOGGER_BUFFER_SIZE 50
#endif

#if CLOGGER_MAX_MESSAGE_SIZE < 10
#error "Logger max message size must be 10 or greater"
#endif

#if CLOGGER_ID_MAX_LEN < 1
#error "Number of characters for log id must be at least 1"
#endif

#if CLOGGER_MAX_NUM_HANDLERS < 1
#error "There must be at least one handler"
#elif CLOGGER_MAX_NUM_HANDLERS <= 8
typedef uint8_t t_handlerref;
#elif CLOGGER_MAX_NUM_HANDLERS <= 16
typedef uint16_t t_handlerref;
#elif CLOGGER_MAX_NUM_HANDLERS <= 32
typedef uint32_t t_handlerref;
#else
#error "Can't support more than 32 handlers"
#endif


// ################ HANDLER CODE ################
t_handlerref logger_create_console_handler(FILE *p_pOut);
t_handlerref logger_create_file_handler(const char* p_sLogLocation, const char* p_sLogName);

#ifdef CLOGGER_GRAYLOG
#define GRAYLOG_TCP 0
#define GRAYLOG_UDP 1
t_handlerref logger_create_graylog_handler(const char* p_sServer, int p_nPort, int p_nProtocol);
#endif


// ################ ID CODE ################
typedef uint8_t logger_id;
extern const logger_id CLOGGER_DEFAULT_ID;
extern const t_handlerref CLOGGER_HANDLER_ERR;

logger_id logger_create_id(const char* p_sID);
logger_id logger_create_id_w_handlers(const char* p_sID, t_handlerref p_refHandlers);
int logger_remove_id(logger_id id_ref);



// ################ Logger Code ################

/*!
 * Initializes variables and starts the logger thread.
 *
 * Returns 0 on success
 *
 */
int logger_init(int p_nLogLevel);

/*!
 * Tells the logger thread it's time to exit, then frees any
 * used memory.
 *
 * Returns 0 on success
 *
 */
int logger_free();

/*!
 * Log a message to all available handlers using the default logger_id.
 *
 * Returns 0 on success
 *
 */
int logger_log_msg(int p_nLogLevel, const char* msg, ...);

/*!
 * Log a message to all available handlers using the specified logger_id.
 *
 * Returns 0 on success
 *
 */
int logger_log_msg_id(int p_nLogLevel, logger_id log_id, const char* msg, ...);

/*!
 * Returns the integer representation of the log level specified
 * by the string p_sLogLevel.
 *
 * Returns a negative value on failure
 *
 */
int logger_log_str_to_int(const char* p_sLogLevel);

/*!
 * Returns an int indicating if the logger is active.
 *
 * Returns > 0 if logger is running, 0 if it isn't
 *
 */
int logger_is_running();

#ifndef CLOGGER_NO_SLEEP
int logger_set_sleep_time(unsigned int p_nMillisecondsToSleep);
#endif

#ifdef __cplusplus
}
#endif

#endif

