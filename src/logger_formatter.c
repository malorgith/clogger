
#include "logger_formatter.h"

#include "logger_levels.h"

#include <stdlib.h>
#include <string.h>

// global variables
static const char* g_sDefaultDateFormat = { "%Y/%m/%d" };
/*
 * TODO implement or remove items below
static const char* lgf_date_only = { "%Y/%m/%d" };
static const char* lgf_time_only = { "%h:%M:%s" };
*/

// private function declarations
int _lgf_obj_check(logger_formatter* formatobj);

// private function definitions
int _lgf_obj_check(logger_formatter* formatobj) {
    if (formatobj == NULL) {
        lgu_warn_msg("Format object is null.");
        return 1;
    }
    else if (formatobj->obj_not_init) {
        lgu_warn_msg("Format object has not been initialized.");
        return 1;
    }
    else if (formatobj->lock == NULL) {
        lgu_warn_msg("Format object missing lock. Has it already been freed?");
        return 1;
    }

    return 0;
}

// public functions
int lgf_init(logger_formatter* formatobj) {

    formatobj->date_time_enabled = false;
    formatobj->m_cSeperator = FORMATTER_SEP_SPACE;

    formatobj->lock = (sem_t*) malloc(sizeof(sem_t));
    sem_init(formatobj->lock, 0, 1);
    formatobj->obj_not_init = 0;

    int snprintf_rtn = snprintf(formatobj->date_format, FORMATTER_DATE_FORMAT_SIZE, "%s", g_sDefaultDateFormat);
    if ((snprintf_rtn >= FORMATTER_DATE_FORMAT_SIZE) || (snprintf_rtn < 0)) {
        lgu_warn_msg("Failed to store the date format.");
        return 1;
    }

    formatobj->level_code_format = lgl_ustrs;
    formatobj->max_level_len = lgl_get_max_len(formatobj->level_code_format);
    if (formatobj->max_level_len < 0) {
        lgu_warn_msg("Failed to determine the length of the error codes.");
        return 1;
    }

    return 0;
}

int lgf_free(logger_formatter* formatobj) {

    sem_wait(formatobj->lock);

    formatobj->level_code_format = NULL;

    sem_destroy(formatobj->lock);
    free(formatobj->lock);
    formatobj->lock = NULL;

    return 0;
}

/*
 * TODO implement or remove function
int lgf_enable_date(logger_formatter* formatobj, char* p_sDateFormat) {

    const char* t_sFormatToUse = g_sDefaultDateFormat;
    if (p_sDateFormat != NULL) {
        t_sFormatToUse = p_sDateFormat;
    }

    if (lgf_set_date_format(t_sFormatToUse, formatobj) != 0) {
        lgu_warn_msg("Failed to set the date format.");
        return 1;
    }
    formatobj->date_time_enabled = true;
    return 0;
}
*/

int lgf_format(logger_formatter* formatobj, char* dest, struct tm *tm, int lglevel) {

    if (_lgf_obj_check(formatobj)) {
        return 1;
    }
    else if (tm == NULL) {
        lgu_warn_msg("Given an invalid object to format");
        return 1;
    }
    else if (dest == NULL) {
        lgu_warn_msg("Destination to place format cannot be null");
        return 1;
    }
    else if (lgl_check(lglevel)) {
        lgu_warn_msg_int("Log level has invalid range; value: %d", lglevel);
        return 1;
    }

    // get the lock
    sem_wait(formatobj->lock);

    // create the date/time string
    char formatted_date[FORMATTER_DATE_SIZE];
    // TODO 0 isn't necessarily an error; it _can_ mean error, or it can mean 0 bytes written, which can be valid
    if (strftime(formatted_date, FORMATTER_DATE_SIZE, formatobj->date_format, tm) == 0) {
        // strftime() did not write contents to the string
        lgu_warn_msg("Failed to format the date and time string.");
        sem_post(formatobj->lock);
        return 1;
    }

    // get the log level string
    int len_to_allocate = formatobj->max_level_len + 1;
    char lvl_str[len_to_allocate];
    snprintf(lvl_str, len_to_allocate, "%s", formatobj->level_code_format[lglevel]);
    for (int count = strlen(lvl_str); count < len_to_allocate - 1; count++)
        lvl_str[count] = ' ';
    lvl_str[len_to_allocate - 1] = '\0';

    // put it all together
    // TODO size below should be variable
    snprintf(dest, 50, "%s %s ", formatted_date, lvl_str);

    sem_post(formatobj->lock);

    return 0;
}

int lgf_get_date(char* p_sDate, logger_formatter* formatobj) {

    if ((p_sDate == NULL) || (formatobj == NULL) || (formatobj->date_format == NULL)) {
        lgu_warn_msg("Log Formatter is missing required data for the date.");
        return 1;
    }

    if (!formatobj->date_time_enabled) {
        // is the line below safer?
//      snprintf(p_sDate, 2, "%c", '\0');
        p_sDate[0] = '\0';
        return 0;
    }

    // get the date
    {
        time_t t_Time = time(NULL);
        struct tm t_TimeData;
        struct tm *t_pResult = localtime_r(&t_Time, &t_TimeData);
        if (t_pResult != &t_TimeData) {
            lgu_warn_msg("log formatter failed to get the time.");
            return 1;
        }

        if (strftime(p_sDate, FORMATTER_DATE_SIZE, formatobj->date_format, t_pResult) == 0) {
            // strftime() did not write contents to the string
            lgu_warn_msg("Failed to format the date and time string.");
            return 1;
        }
    }

    return 0;
}

int lgf_get_format(char* p_sString, logger_formatter* formatobj) {

    char t_sDate[FORMATTER_DATE_SIZE];
    if (lgf_get_date(t_sDate, formatobj) != 0)
        return 1;

    if (snprintf(p_sString, FORMATTER_MAX_TOTAL_SIZE, "%s", t_sDate) >= FORMATTER_MAX_TOTAL_SIZE) {
        lgu_warn_msg("The final size of the format is too large.");
        return 1;
    }

    return 0;
}

int lgf_set_datetime_format(logger_formatter* formatobj, const char* datetime_format) {

    if (_lgf_obj_check(formatobj)) {
        return 1;
    }
    else if (datetime_format == NULL) {
        lgu_warn_msg("Cannot set new datetime: string is null.");
        return 1;
    }

    sem_wait(formatobj->lock);
    int snprintf_rtn = snprintf(formatobj->date_format, FORMATTER_DATE_FORMAT_SIZE, "%s", datetime_format);
    if ((snprintf_rtn >= FORMATTER_DATE_FORMAT_SIZE) || (snprintf_rtn < 0)) {
        lgu_warn_msg("Failed to copy the datetime format into the object.");
        return 1;
    }
    sem_post(formatobj->lock);

    return 0;

}

/*
 * TODO implement or remove functions
int lgf_get_format_no_date(char* p_sString, logger_formatter* formatobj) {

    return 0;
}

int lgf_set_date_format(char* p_sDateFormat, logger_formatter* formatobj) {

    if (snprintf(formatobj->date_format, FORMATTER_DATE_FORMAT_SIZE, "%s", p_sDateFormat) >= FORMATTER_DATE_FORMAT_SIZE) {
        lgu_warn_msg("The date format is too large to save.");
        return 1;
    }
//  formatobj->date_time_enabled = true;
    return 0;
}

int lgf_set_date_only(logger_formatter* formatobj) {
    return (lgf_set_datetime_format(formatobj, lgf_date_only));
}
int lgf_set_no_datetime(logger_formatter* formatobj);

int lgf_set_time_only(logger_formatter* formatobj) {
    return (lgf_set_datetime_format(formatobj, lgf_time_only));
}
*/

