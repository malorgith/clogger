
#include "logger_util.h"

#include <stdarg.h> // va_list and others
#include <stdio.h> //fprintf(), snprintf()
#include <sys/stat.h>
#include <unistd.h>


// public functions
int lgu_add_to_time(struct timespec *p_pTspec, int ms_to_add, int min_ms, int max_ms) {

    if (ms_to_add > max_ms) {
        lgu_warn_msg_int("Maximum time to wait is %d milliseconds", max_ms);
        ms_to_add = max_ms;
    }
    else if (ms_to_add < min_ms) {
        lgu_warn_msg_int("Minimum time to wait is %d milliseconds", min_ms);
        ms_to_add = min_ms;
    }

    // convert the milliseconds to nanoseconds
    long sleep_time_nsecs = ms_to_add;
    int sleep_time_secs = 0;
    while (sleep_time_nsecs >= 1000) {
        sleep_time_nsecs -= 1000;
        sleep_time_secs++;
    }
    sleep_time_nsecs *= (long) 1000000;

    // add the time to the struct
    p_pTspec->tv_sec += sleep_time_secs;
    p_pTspec->tv_nsec += sleep_time_nsecs;
    if (p_pTspec->tv_nsec >= (long) 1000000000) {
        p_pTspec->tv_nsec -= (long) 1000000000;
        p_pTspec->tv_sec += 1;
    }

    return 0;
}

int lgu_can_write(const char* p_sFileWithPath) {

    int t_nRtn = access(p_sFileWithPath, W_OK);
    return t_nRtn;
}

int lgu_get_size(const char* p_sFilename) {

    // get the size (in bytes) of the file

    struct stat t_statStruct;
    if (stat(p_sFilename, &t_statStruct) == -1) {
        return -1;
    }
    else {
        // get the size from the stat struct
        return t_statStruct.st_size;
    }

    // shouldn't be possible to get here
    return -1;
}

int lgu_is_dir(const char* p_sDirectory) {

    struct stat sb;

    if (stat(p_sDirectory, &sb) == -1)
        return -1;
    else
        if (S_ISDIR(sb.st_mode))
            return 0;
        else
            return 1;

}

bool lgu_is_file(const char* p_sFilename) {

	struct stat t_statBuf;
	if (stat(p_sFilename, &t_statBuf) == -1)
	    return false;
	else
	    if (S_ISREG(t_statBuf.st_mode))
	        return true;
	    else
	        return false;

}

/*
 * snprintf() that checks the result and prints a warning message (using
 * lgu_warn_msg()) if string failed to copy.
 */
int lgu_wsnprintf(char* p_sDest, int p_nMaxSize, const char* p_sWarnMsg, const char* p_sFormat, ...) {
    va_list t_listArgs;
    va_start(t_listArgs, p_sFormat);
    int t_nSnrtn = vsnprintf(p_sDest, p_nMaxSize, p_sFormat, t_listArgs);
    va_end(t_listArgs);

    if ((t_nSnrtn < 0) || (t_nSnrtn >= p_nMaxSize)) {
        lgu_warn_msg(p_sWarnMsg);
        return 1;
    }

    return 0;
}

#ifdef CLOGGER_VERBOSE_WARNING
/*
 * Print the error message to stderr
 */
void lgu_warn_msg(const char* p_sMsg) {
    fprintf(stderr, "clogger error: %s\n", p_sMsg);
    fflush(stderr);
}

void lgu_warn_msg_int(const char* p_sMsgFormat, const int p_nPut) {
    static const int t_nMaxMsgSize = 300;
    char t_sMsg[t_nMaxMsgSize];

    int t_nFormatRtn = snprintf(t_sMsg, t_nMaxMsgSize, p_sMsgFormat, p_nPut);
    if ((t_nFormatRtn == 0) || (t_nFormatRtn >= t_nMaxMsgSize)) {
        // failed to format the message
        lgu_warn_msg("Encountered an error, but failed to format output.");
    }
    else {
        lgu_warn_msg(t_sMsg);
    }
}

void lgu_warn_msg_str(const char *p_sMsgFormat, const char *p_sPut) {
    static const int t_nMaxMsgSize = 300;
    char t_sMsg[t_nMaxMsgSize];

    int t_nFormatRtn = snprintf(t_sMsg, t_nMaxMsgSize, p_sMsgFormat, p_sPut);
    if ((t_nFormatRtn == 0) || (t_nFormatRtn >= t_nMaxMsgSize)) {
        // failed to format the message
        lgu_warn_msg("encountered an error, but failed to format output.");
    }
    else {
        lgu_warn_msg(t_sMsg);
    }
}
#else
/*
 * Ignore the message
 */
void lgu_warn_msg(__attribute__((unused))const char* p_sMsg) {}
void lgu_warn_msg_int(__attribute__((unused))const char* p_sMsgFormat, __attribute__((unused))const int p_nPut) {}
void lgu_warn_msg_str(__attribute__((unused))const char* p_sMsgFormat, __attribute__((unused))const char *p_sPut) {}
#endif

