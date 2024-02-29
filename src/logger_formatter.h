#ifndef MALORGITH_CLOGGER_FORMATTER_H_
#define MALORGITH_CLOGGER_FORMATTER_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include <stdbool.h>
#include <semaphore.h>
#include <time.h>

#include "logger_defines.h"
#include __MALORGITH_CLOGGER_INCLUDE

#define FORMATTER_DATE_SIZE 30
#define FORMATTER_DATE_FORMAT_SIZE 40

#define FORMATTER_MAX_TOTAL_SIZE FORMATTER_DATE_SIZE

__MALORGITH_NAMESPACE_OPEN

extern char const kCloggerFormatterSeparatorBracket;
extern char const kCloggerFormatterSeparatorSpace;

typedef struct {
    bool            date_time_enabled_;
    char            date_format_[FORMATTER_DATE_FORMAT_SIZE];
    char const**    level_code_format_;
    sem_t*          lock_;
    char            seperator_;
    int             max_level_len_;
    int             obj_no_init_;
} logger_formatter;

// TODO implement or remove items below
//extern char const* lgf_date_only;
//extern char const* lgf_time_only;
//extern char const* g_sDefaultDateFormat;

int lgf_init(logger_formatter* formatobj);
int lgf_free(logger_formatter* formatobj);

int lgf_format(logger_formatter* formatobj, char* dest, struct tm *tm, int lglevel);

int lgf_set_date_only(logger_formatter* formatobj);
int lgf_set_datetime_format(logger_formatter* formatobj, char const* datetime_format);
int lgf_set_no_datetime(logger_formatter* formatobj);
int lgf_set_time_only(logger_formatter* formatobj);


/*
 * TODO implement or remove functions below
int lgf_get_format(char* dest, logger_formatter* formatobj);
int lgf_get_format_no_date(char* dest, logger_formatter* formatobj);

int lgf_enable_date(logger_formatter* formatobj, char* date_format);

int lgf_get_date(char* date_str, logger_formatter* formatobj);

int lgf_set_date_format(char* date_format, logger_formatter* formatobj);
*/

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_FORMATTER_H_
