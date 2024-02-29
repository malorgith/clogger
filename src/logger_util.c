
#include "logger_util.h"

#include <errno.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

__MALORGITH_NAMESPACE_OPEN

/*!
 * @brief Get the number of bytes in the file.
 *
 * @returns the number of bytes in the file, < 0 on failure
 */
static int _lgu_get_size(
    /*! the path to the file to get the size of */
    char const* str_file_path
);

/*!
 * @brief Check if the given path is a file.
 *
 * @returns true if the path is to a file, false otherwise
 */
static bool _lgu_is_file(
    /*! the path to the file */
    char const* str_file_path
);

// public functions
int lgu_add_to_time(
    struct timespec *ptr_timespec,
    int ms_to_add,
    int min_ms,
    int max_ms
) {

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
    ptr_timespec->tv_sec += sleep_time_secs;
    ptr_timespec->tv_nsec += sleep_time_nsecs;
    if (ptr_timespec->tv_nsec >= (long) 1000000000) {
        ptr_timespec->tv_nsec -= (long) 1000000000;
        ptr_timespec->tv_sec += 1;
    }

    return 0;
}

int lgu_can_write(char const* str_file_path) {
    int return_val = access(str_file_path, W_OK);
    return return_val;
}

__attribute__((unused))int _lgu_get_size(char const* str_file_path) {
    // get the size (in bytes) of the file
    struct stat t_statStruct;
    if (stat(str_file_path, &t_statStruct) == -1) {
        return -1;
    }
    else {
        // get the size from the stat struct
        return t_statStruct.st_size;
    }
    // shouldn't be possible to get here
    return -1;
}

int lgu_is_dir(char const* str_dir_path) {
    struct stat stat_struct;
    if (stat(str_dir_path, &stat_struct) == -1) {
        return -1;
    }
    else {
        if (S_ISDIR(stat_struct.st_mode)) {
            return 0;
        }
        else {
            return 1;
        }
    }
}

__attribute__((unused))bool _lgu_is_file(char const* str_file_path) {
	struct stat stat_struct;
	if (stat(str_file_path, &stat_struct) == -1) {
	    return false;
    }
	else {
	    if (S_ISREG(stat_struct.st_mode)) {
	        return true;
        }
	    else {
	        return false;
        }
    }
}

int lgu_timedwait(sem_t *sem_ptr, int milliseconds_to_wait) {
    static const int max_milliseconds_wait = 3000;
    static const int min_milliseconds_wait = 1;

    struct timespec t_timespecWait;
    if (clock_gettime(CLOCK_REALTIME, &t_timespecWait) == 1) {
        lgu_warn_msg("failed to get the time before waiting for semaphore");
        return -1;
    }

    lgu_add_to_time(&t_timespecWait, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);

    int t_nSemRtn;
    while((t_nSemRtn = sem_timedwait(sem_ptr, &t_timespecWait)) == -1 && errno == EINTR)
        continue;

    if (t_nSemRtn == -1) {
        // did not return 0; indicates an error
        if (errno != ETIMEDOUT) {
            // it didn't timeout, something else went wrong
            perror("sem_timedwait");
            return -1;
        }
        else {
            // it timed out
            lgu_warn_msg("timed out trying to get semaphore");
            return 1;
        }
    }

    return 0;  // didn't timeout
}

/*
 * snprintf() that checks the result and prints a warning message (using
 * lgu_warn_msg()) if string failed to copy.
 */
int lgu_wsnprintf(
    char* dest,
    int max_size,
    char const* str_warn_msg,
    char const* format,
    ...
) {
    va_list list_args;
    va_start(list_args, format);
    int sn_rtn = vsnprintf(dest, max_size, format, list_args);
    va_end(list_args);

    if ((sn_rtn < 0) || (sn_rtn >= max_size)) {
        lgu_warn_msg(str_warn_msg);
        return 1;
    }

    return 0;
}

#ifdef CLOGGER_VERBOSE_WARNING
/*
 * Print the error message to stderr
 */
void lgu_warn_msg(char const* msg) {
    fprintf(stderr, "clogger error: %s\n", msg);
    fflush(stderr);
}

void lgu_warn_msg_int(char const* msg_format, const int int_input) {
    static const int max_msg_size = 300;
    char out_msg[max_msg_size];

    int format_rtn = snprintf(out_msg, max_msg_size, msg_format, int_input);
    if ((format_rtn == 0) || (format_rtn >= max_msg_size)) {
        // failed to format the message
        lgu_warn_msg("Failed to format error output.");
    }
    else {
        lgu_warn_msg(out_msg);
    }
}

void lgu_warn_msg_str(char const* msg_format, char const* str_input) {
    static const int max_msg_size = 300;
    char out_msg[max_msg_size];

    int format_rtn = snprintf(out_msg, max_msg_size, msg_format, str_input);
    if ((format_rtn == 0) || (format_rtn >= max_msg_size)) {
        // failed to format the message
        lgu_warn_msg("Failed to format error output.");
    }
    else {
        lgu_warn_msg(out_msg);
    }
}
#else
/* ignore the message */
void lgu_warn_msg(__attribute__((unused))char const* msg) {}
void lgu_warn_msg_int(__attribute__((unused))char const* format, __attribute__((unused))const int input_int) {}
void lgu_warn_msg_str(__attribute__((unused))char const* format, __attribute__((unused))char const* input_str) {}
#endif // CLOGGER_VERBOSE_WARNING

__MALORGITH_NAMESPACE_CLOSE
