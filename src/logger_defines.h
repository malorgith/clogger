#ifndef MALORGITH_CLOGGER_DEFINES_H_
#define MALORGITH_CLOGGER_DEFINES_H_

#ifdef __cplusplus
#pragma once
#define __MALORGITH_NAMESPACE_OPEN namespace malorgith { namespace clogger {
#define __MALORGITH_NAMESPACE_CLOSE } }
#define __MALORGITH_CLOGGER_INCLUDE "clogger.hpp"
#else
#define __MALORGITH_NAMESPACE_OPEN
#define __MALORGITH_NAMESPACE_CLOSE
#define __MALORGITH_CLOGGER_INCLUDE "clogger.h"
#endif  // __cplusplus

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

#if LOGGER_MAX_LEVEL < LOGGER_DEBUG
#error "Logger max level must be greater than or equal to debug level"
#elif LOGGER_MAX_LEVEL < 7
#error "There must be at least 8 log levels supported"
#endif  // LOGGER_MAX_LEVEL < LOGGER_DEBUG

#ifndef CLOGGER_MAX_MESSAGE_SIZE
#define CLOGGER_MAX_MESSAGE_SIZE 200
#endif  // CLOGGER_MAX_MESSAGE_SIZE

#if CLOGGER_MAX_MESSAGE_SIZE < 10
#error "Logger max message size must be 10 or greater"
#endif  // CLOGGER_MAX_MESSAGE_SIZE

#ifndef CLOGGER_ID_MAX_LEN
#define CLOGGER_ID_MAX_LEN   30
#endif  // CLOGGER_ID_MAX_LEN

#if CLOGGER_ID_MAX_LEN < 5
#error "Number of characters for log id must be at least 1"
#endif  // CLOGGER_ID_MAX_LEN

#ifndef CLOGGER_MAX_NUM_IDS
#define CLOGGER_MAX_NUM_IDS 20
#endif  // CLOGGER_MAX_NUM_IDS

#if CLOGGER_MAX_NUM_IDS < 3
#error "There must be room for at least 3 IDs"
#endif  // CLOGGER_MAX_NUM_IDS

#ifndef CLOGGER_MAX_NUM_HANDLERS
#define CLOGGER_MAX_NUM_HANDLERS 5
#endif  // CLOGGER_MAX_NUM_HANDLERS

#if CLOGGER_MAX_NUM_HANDLERS < 2
#error "There must be space for at least 2 handlers"
#endif  // CLOGGER_MAX_NUM_HANDLERS

#ifndef CLOGGER_BUFFER_SIZE
#define CLOGGER_BUFFER_SIZE 50
#endif  // CLOGGER_BUFFER_SIZE

#if CLOGGER_BUFFER_SIZE < 10
#error "Buffer size must be at least 10"
#endif  // CLOGGER_BUFFER_SIZE

#ifndef CLOGGER_MAX_FORMAT_SIZE
#define CLOGGER_MAX_FORMAT_SIZE 50
#endif  // CLOGGER_MAX_FORMAT_SIZE

#if CLOGGER_MAX_FORMAT_SIZE < 30
#error "The format size must be at least 30"
#endif  // CLOGGER_MAX_FORMAT_SIZE

extern int const kCloggerMaxFormatSize;

#endif  // MALORGITH_CLOGGER_DEFINES_H_
