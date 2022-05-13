
#ifndef FORMATTER_H_INCLUDED
#define FORMATTER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "clogger.h"

#include <stdbool.h>
#include <semaphore.h>
#include <time.h>

#define FORMATTER_DATE_SIZE 30
#define FORMATTER_DATE_FORMAT_SIZE 40

#define FORMATTER_MAX_TOTAL_SIZE FORMATTER_DATE_SIZE

#define FORMATTER_SEP_BRACKET   '['
#define FORMATTER_SEP_SPACE     ' '

typedef struct {
    bool            date_time_enabled;
    char            date_format[FORMATTER_DATE_FORMAT_SIZE];
    const char**    level_code_format;
    sem_t*          lock;
    char            m_cSeperator;
    int             max_level_len;
    int             obj_not_init;
} logger_formatter;

// TODO implement or remove items below
//extern const char* lgf_date_only;
//extern const char* lgf_time_only;
//extern const char* g_sDefaultDateFormat;

int lgf_init(logger_formatter* formatobj);
int lgf_free(logger_formatter* formatobj);

int lgf_format(logger_formatter* formatobj, char* dest, struct tm *tm, int lglevel);

int lgf_set_date_only(logger_formatter* formatobj);
int lgf_set_datetime_format(logger_formatter* formatobj, const char* datetime_format);
int lgf_set_no_datetime(logger_formatter* formatobj);
int lgf_set_time_only(logger_formatter* formatobj);


/*
 * TODO implement or remove functions below
int lgf_get_format(char* p_sString, logger_formatter* formatobj);
int lgf_get_format_no_date(char* p_sString, logger_formatter* formatobj);

int lgf_enable_date(logger_formatter* formatobj, char* p_sDateFormat);

int lgf_get_date(char* p_sDate, logger_formatter* formatobj);

int lgf_set_date_format(char* p_sDateFormat, logger_formatter* formatobj);
*/

#ifdef __cplusplus
}
#endif

#endif
