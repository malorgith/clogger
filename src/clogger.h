
#ifndef CLOGGER_H_INCLUDED
#define CLOGGER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // FILE

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

#ifndef LOGGER_MAX_MESSAGE_SIZE
#define LOGGER_MAX_MESSAGE_SIZE 200
#endif

#ifndef LOGGER_ID_MAX_LEN
#define LOGGER_ID_MAX_LEN   30
#endif

#ifndef CLOGGER_MAX_NUM_HANDLERS
#define CLOGGER_MAX_NUM_HANDLERS 5
#endif

/*
 * In theory the compiler should catch any of the conditions below
 * based on items that are allocated using these variables, but we're
 * going to make sure.
 */

#if LOGGER_MAX_MESSAGE_SIZE < 10
#error "Logger max message size must be 10 or greater"
#endif

#if LOGGER_ID_MAX_LEN < 1
#error "Number of characters for log id must be at least 1"
#endif

#if CLOGGER_MAX_NUM_HANDLERS < 1
#error "There must be at least one handler"
#endif


// ################ HANDLER CODE ################
int logger_create_console_handler(FILE *p_pOut);
int logger_create_file_handler(char* p_sLogLocation, char* p_sLogName);

#ifdef CLOGGER_GRAYLOG
#define GRAYLOG_TCP 0
#define GRAYLOG_UDP 1
int logger_create_graylog_handler(char* p_sServer, int p_nPort, int p_nProtocol);
#endif


// ################ ID CODE ################
typedef int logger_id;
extern const logger_id CLOGGER_DEFAULT_ID;
logger_id logger_create_id(char* p_sID);
int logger_print_id(logger_id id_ref); // TODO consider moving to non-public or testing functions
int logger_remove_id(logger_id id_ref);


// ################ Formatter Code ################
typedef uint8_t logger_formatter_id;
logger_formatter_id logger_create_formatter();


// ################ Logger Code ################

/*!
 * Initializes variables and starts the logger thread.
 * 
 */
bool logger_init(int p_nLogLevel);

/*!
 * Tells the logger thread it's time to exit, then frees any
 * used memory.
 * 
 */
bool logger_free();

/*!
 * Log a message to all available handlers using the default logger_id.
 *
 */
bool logger_log_msg(int p_nLogLevel, char* msg, ...);

/*!
 * Log a message to all available handlers using the specified logger_id.
 *
 */
bool logger_log_msg_id(int p_nLogLevel, logger_id log_id, char* msg, ...);

/*!
 * Returns the integer representation of the log level specified
 * by the string p_sLogLevel.
 *
 */
int logger_log_str_to_int(char* p_sLogLevel);

/*!
 * Returns a bool indicating if the logger is active.
 *
 */
bool logger_is_running();

#ifdef __cplusplus
}
#endif

#endif

