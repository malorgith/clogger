
#include "logger.h"

#include "handlers/console_handler.h"
#include "handlers/file_handler.h"
#include "logger_buffer.h"
#include "logger_formatter.h"
#ifdef CLOGGER_GRAYLOG
#include "handlers/graylog_handler.h"
#endif

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>     // for strcmp

#define LOGGER_SLEEP_SECS 1

// global variables
static atomic_bool g_bExit = { false };
static bool volatile g_logInit = { false };
static int g_nLogLevel;
static pthread_t g_LogThread;

static logger_formatter* g_lgformatter = { NULL };

static int buf_refid = { -1 };

// TODO value below should be removed or determined based on what the handlers require
static bool volatile g_bTimestampEnabled = { true };

const logger_id CLOGGER_DEFAULT_ID = { 0 };

// private function declarations
static int _logger_add_message(t_loggermsg* msg);
static int _logger_log_msg(
    int log_level,
    logger_id id,
    char* format,
    char* msg,
    va_list arg_list
);
static int _logger_read_message();
static void *_logger_run(void *p_pData);
static int _logger_timedwait(sem_t *p_pSem, int t_nWaitTimeSecs);

// private function definitions
int _logger_add_message(t_loggermsg* msg) {

    if (!g_logInit) {
        lgu_warn_msg("Dropping message because logger isn't running.");
        free(msg);
        return 1;
    }
    /*
     * Allow messages to be added to the buffer when there are no
     * handlers IF the logger has been initialized (above).
     */

    if (lgb_add_message(buf_refid, msg)) {
        lgu_warn_msg("Logger failed to add message to buffer.");
        return 1;
    }

    return 0;

}

int _logger_log_msg(
    int log_level,
    logger_id id,
    __attribute__((unused))char* format,
    char* msg,
    va_list arg_list
) {

    // TODO implement format parameter

    // Check the level
    if (log_level < 0) {
        lgu_warn_msg("Log level must be at least zero");
        return 1;
    }
    else if (log_level > g_nLogLevel) {
        // This message won't be logged based on the log level
        return 0; // return 0 because no error occurred
    }
    else if (!g_logInit) {
        lgu_warn_msg("Can't add message; logger isn't running.");
        return 1;
    }

    t_loggermsg* t_sFinalMessage = (t_loggermsg*) malloc(sizeof(t_loggermsg));
    va_list arg_list_copy;
    va_copy(arg_list_copy, arg_list);
    int format_rtn = vsnprintf(t_sFinalMessage->m_sMsg, CLOGGER_MAX_MESSAGE_SIZE, msg, arg_list_copy);
    va_end(arg_list_copy);
    if ((format_rtn >= CLOGGER_MAX_MESSAGE_SIZE) || (format_rtn < 0)) {
        lgu_warn_msg("Failed to format the message before adding it to the buffer.");
        free(t_sFinalMessage);
        return 1;
    }
    t_sFinalMessage->m_nLogLevel = log_level;
    t_sFinalMessage->m_nId = id;
    t_sFinalMessage->m_sFormat = NULL;

    /*
     * TODO
     * We currently have to get the logger_id here in case it's removed before the logging
     * thread gets to the message on the buffer. This isn't ideal because we must obtain locks
     * to get the value. Instead each logger_id should have its own lock(s) that can be used
     * to indicate when it currently has an associated message on a buffer, and we will
     * simply increment a lock here to indicate it can't be removed.
     */
    char t_sId[CLOGGER_ID_MAX_LEN];
    if (lgi_get_id(t_sFinalMessage->m_nId, t_sId) != 0) {
        lgu_warn_msg("Failed to convert ID from reference to string.");
        free(t_sFinalMessage);
        return 1;
    }
    int copy_rtn = snprintf(t_sFinalMessage->m_sId, CLOGGER_ID_MAX_LEN * (sizeof(char)), "%s", t_sId);
    if ((copy_rtn >= CLOGGER_ID_MAX_LEN) || (copy_rtn < 0)) {
        lgu_warn_msg("Failed to set the logger ID in the message.");
        free(t_sFinalMessage);
        return 1;
    }

    /*
     * TODO
     * Add support for querying all set handlers to see if any require a timestamp.
     * If no handlers need one, then we don't need to set the data in the message struct.
     * Right now we assume at least one handler requires a timestamp.
     */

    if (g_bTimestampEnabled) {
        time_t t_Time = time(NULL);
        struct tm t_TimeData;
        struct tm *t_pResult = localtime_r(&t_Time, &t_TimeData);
        if (t_pResult != &t_TimeData) {
            lgu_warn_msg("logger failed to get the time.");
            free(t_sFinalMessage);
            return 1;
        }
        t_sFinalMessage->m_tmTime = t_TimeData;
    }

    return (_logger_add_message(t_sFinalMessage));
}

int _logger_read_message() {

    if(!g_logInit)
        return 1;
    else if (lgh_get_num_handlers() < 1)
        return 1;

    // Get the message at the current index
    t_loggermsg* t_pMsg = lgb_read_message(buf_refid);
    if (t_pMsg == NULL) {
        lgu_warn_msg("Failed to read a message from the buffer.");
        return 1;
    }

    // get the format string
    char formatted_string[50]; // FIXME size
    if (lgf_format(g_lgformatter, formatted_string, &t_pMsg->m_tmTime, t_pMsg->m_nLogLevel)) {
        lgu_warn_msg("Failed to get the format for the message.");
        free(t_pMsg);
        return 1;
    }
    t_pMsg->m_sFormat = formatted_string;

    /*
     * TODO
     * We should get the value of the logger_id here, but it could lead to an ID being
     * removed before the logging thread retrieves its value. Locks should be added to
     * each ID to indicate when it's currently waiting to be used. When those are in place,
     * we can can get the value of the ID here, and free its lock when done.
     */

    // TODO attach handlers to logger_id, and write to handlers based on the id used
    if (lgh_write_to_all(t_pMsg)) {
        // we either failed to write to one or more handlers, or there were no open
        // handlers to write to
        lgu_warn_msg("logger thread failed to write to a handler");
    }
    free(t_pMsg);                   // free the memory

    return 0;
}

void *_logger_run(__attribute__((unused))void *p_pData) {

    bool t_bExit = false;
    int t_nCurrentHandlers = 0;
    while(true) {

        short t_nMessagesBeforeCheck = 25;  // TODO This value should probably be less than the buffer size
        short t_nMessagesRead = 0;

        while(t_nMessagesRead < t_nMessagesBeforeCheck) {
            int wait_rtn = lgb_wait_for_messages(buf_refid, 1); // FIXME need to make this variable

            int t_nNewHandlerCount = lgh_get_num_handlers();
            if (t_nNewHandlerCount < 0) {
                // FIXME error handle
                lgu_warn_msg("logger thread aborting because it couldn't get number of handlers");
                return NULL;
            }

            if (t_nNewHandlerCount != t_nCurrentHandlers) {
                // handler(s) to open
                if (lgh_open_handlers()) {
                    // failed to open a handler
                    lgu_warn_msg("Logger thread failed to open a handler.");
                    // FIXME error handle
                    return NULL;
                }
                t_nCurrentHandlers= lgh_get_num_handlers();
            }

            if (wait_rtn == 0) {
                // there's a message to read
                _logger_read_message();
                t_nMessagesRead++;
            }
            else if (wait_rtn > 0) {
                // timed out; break from the loop to check if we need to exit
                break;
            }
            else if (wait_rtn < 0) {
                // something went wrong
                lgu_warn_msg("Something went wrong while waiting for a message to be placed on the buffer.");
                // FIXME fail? error?
                break;
            }
        }

        if (t_bExit)
            // We've already detected that it's time to leave and gone through the loop an additional time
            break;

        // Check if it's time to exit
        if (g_bExit) {
            // set t_bExit to true; we will loop one more time to get any remaining messages
            t_bExit = true;
        }
    }

    if (lgh_remove_all_handlers()) {
        // at least one handler failed to close successfully
        lgu_warn_msg("encountered error closing a handler");
    }

    bool* rtn_val = (bool*) malloc(sizeof(bool));
    *rtn_val = true;
    pthread_exit(rtn_val);

}

/*
 * TODO
 * This function was used when the handlers were stored here.
 *
 * It might be worth moving this to a more common area, such as
 * logger_util, so any file that deals with semaphores can use it.
 */
__attribute__((unused))int _logger_timedwait(sem_t *p_pSem, int milliseconds_to_wait) {

    static const int max_milliseconds_wait = 3000;
    static const int min_milliseconds_wait = 1;

    struct timespec t_timespecWait;
    if (clock_gettime(CLOCK_REALTIME, &t_timespecWait) == 1) {
        lgu_warn_msg("Failed to get the time before waiting for semaphore.");
        return 1;
    }

    lgb_add_to_time(&t_timespecWait, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);

    int t_nSemRtn;
    while((t_nSemRtn = sem_timedwait(p_pSem, &t_timespecWait)) == -1 && errno == EINTR)
        continue;

    if (t_nSemRtn == -1) {
        // did not return 0; indicates an error
        if (errno != ETIMEDOUT) {
            // it didn't timeout, something else went wrong
            perror("sem_timedwait");
            return errno;
        }
        else {
            // it timed out
            lgu_warn_msg("Timed out trying to get semaphore.");
            return errno;
        }
    }

    return 0;
}

// public functions
int logger_init(int p_nLogLevel) {

#ifndef NDEBUG
#ifndef CLOGGER_REMOVE_WARNING
    fprintf(stderr, "\n(clogger) WARNING: library not compiled as release build; compile a release build to remove this warning\n\n");
#endif
#endif

    if (g_logInit) {
        lgu_warn_msg("Logger has already been initialized");
        return 1;
    }

    else if (! (LOGGER_SLEEP_SECS > 0)) {
        lgu_warn_msg("The amount of time the logger should sleep must be an integer greater than zero");
        return 1;
    }

    if (lgi_init() != 0) {
        lgu_warn_msg("Failed to initialize the logger IDs.");
        return 1;
    }

    // create the default logger id, which is 'main'
    if (logger_create_id((char*) "main") < 0) {
        lgu_warn_msg("Failed to create default logger ID.");
        return 1;
    }

    // create the formatter
    g_lgformatter = (logger_formatter*) malloc(sizeof(logger_formatter));
    if (g_lgformatter == NULL) {
        lgu_warn_msg("Failed to allocate space for the formatter.");
        return 1;
    }
    if (lgf_init(g_lgformatter)) {
        lgu_warn_msg("Failed to initialize formatter.");
        return 1;
    }
    lgf_set_datetime_format(g_lgformatter, "%Y-%m-%d %H:%M:%S");


    // init the handler
    if (lgh_init()) {
        return 1;
    }

    // Set the log level based on what the user specified
    if (p_nLogLevel < 0) {
        lgu_warn_msg("The log level must be at least zero.");
        return 1;
    }
    g_nLogLevel = p_nLogLevel;

    // TODO make sure this value is >= 0 before changing it?
    buf_refid = lgb_init();
    if (buf_refid < 0) {
        lgu_warn_msg("Failed to create the log buffer.");
        return 1;
    }

    g_bExit = false;

    // Start the log thread
    g_logInit = true;
    pthread_create(&g_LogThread, NULL, _logger_run, NULL);

#ifndef NDEBUG
#ifndef CLOGGER_REMOVE_WARNING
    logger_log_msg(LOGGER_WARN, "Logger started with debugging built in");
#endif
#endif

    return 0;
}

int logger_free() {

    int t_nRtn = 0;

    if (!g_logInit)
        return 1;

    // Tell the logging thread it's time to end
    g_bExit = true;

    bool* join_val = NULL;
    pthread_join(g_LogThread, (void**) &join_val);
    if (join_val == NULL) {
        lgu_warn_msg("Failed to get status from log thread.");
        fflush(stderr);
    }
    else {
        if (*join_val != true) {
            lgu_warn_msg("Got non-true value on exit of log thread.");
            fflush(stderr);
            t_nRtn = 1;
        }
        free(join_val);
    }

    g_logInit = false;
    if (lgb_free()) {
        lgu_warn_msg("Failed to free the log buffer.");
        fflush(stderr);
        t_nRtn = 1;
    }
    buf_refid = -1;

    // free the handler memory
    if (lgh_free()) {
        t_nRtn = 1;
    }

    // free the formatters
    // FIXME semaphore(s) to modify values
    // TODO currently nothing to close(), but might change
    lgf_free(g_lgformatter);
    free(g_lgformatter);
    g_lgformatter = NULL;

    if (lgi_free() != 0) {
        lgu_warn_msg("Failed to free the logger IDs.");
        t_nRtn = 1;
    }

    return t_nRtn;
}

int logger_log_str_to_int(char* p_sLogLevel) {

    if (strcmp(p_sLogLevel, "emergency") == 0) {
        return LOGGER_EMERGENCY;
    }
    else if (strcmp(p_sLogLevel, "alert") == 0) {
        return LOGGER_ALERT;
    }
    else if (strcmp(p_sLogLevel, "critical") == 0) {
        return LOGGER_CRITICAL;
    }
    else if (strcmp(p_sLogLevel, "error") == 0) {
        return LOGGER_ERROR;
    }
    else if(strcmp(p_sLogLevel, "warn") == 0) {
        return LOGGER_WARN;
    }
    else if (strcmp(p_sLogLevel, "notice") == 0) {
        return LOGGER_NOTICE;
    }
    else if (strcmp(p_sLogLevel, "info") == 0) {
        return LOGGER_INFO;
    }
    else if (strcmp(p_sLogLevel, "debug") == 0) {
        return LOGGER_DEBUG;
    }
    else {
        #ifdef CLOGGER_VERBOSE_WARNING
        int t_nMsgSize = 300;
        char t_sLogMsg[t_nMsgSize];
        char* t_sMsg = "Couldn't match specified level '%s' to a known log level";
        int t_nFormatRtn = snprintf(t_sLogMsg, t_nMsgSize, t_sMsg, p_sLogLevel);
        if ((t_nFormatRtn == 0) || (t_nFormatRtn >= t_nMsgSize)) {
            // failed to format the message
            lgu_warn_msg("Couldn't match given string to a known log level");
        }
        else {
            lgu_warn_msg(t_sMsg);
        }
        #endif
        return -1;
    }
}

int logger_is_running() {
    if (g_logInit) return 1;
    else return 0;
}

int logger_log_msg(int p_nLogLevel, char* msg, ...) {
    va_list arg_list;
    va_start(arg_list, msg);
    int rtn_val = _logger_log_msg(
        p_nLogLevel,
        0,   // ID
        NULL,   // format
        msg,
        arg_list
    );
    va_end(arg_list);

    return rtn_val;
}

int logger_log_msg_id(int p_nLogLevel, logger_id log_id, char* msg, ...) {

    va_list arg_list;
    va_start(arg_list, msg);
    int rtn_val = _logger_log_msg(
        p_nLogLevel,
        log_id,   // ID
        NULL,   // format
        msg,
        arg_list
    );
    va_end(arg_list);

    return rtn_val;
}

// handler code
int logger_create_console_handler(FILE *p_pOut) {
    log_handler tmp_handler;
    int rtnval = create_console_handler(&tmp_handler, p_pOut);
    if (rtnval != 0) {
        return rtnval;
    }
    int t_refHandler = lgh_add_handler(&tmp_handler);
    // TODO the references to the handlers should be saved
    if (t_refHandler >= 0) return 0;
    else return 1;
}

int logger_create_file_handler(char* p_sLogLocation, char* p_sLogName) {
    log_handler tmp_handler;
    int rtnval = create_file_handler(&tmp_handler, p_sLogLocation, p_sLogName);
    if (rtnval != 0) {
        return rtnval;
    }
    int t_refHandler = lgh_add_handler(&tmp_handler);
    // TODO the references to the handlers should be saved
    if (t_refHandler >= 0) return 0;
    else return 1;
}

#ifdef CLOGGER_GRAYLOG
int logger_create_graylog_handler(char* p_sServer, int p_nPort, int p_nProtocol) {
    log_handler tmp_handler;
    int rtnval = create_graylog_handler(&tmp_handler, p_sServer, p_nPort, p_nProtocol);
    if (rtnval != 0) {
        return rtnval;
    }
    int t_refHandler = lgh_add_handler(&tmp_handler);
    // TODO the references to the handlers should be saved
    if (t_refHandler >= 0) return 0;
    else return 1;
}
#endif

// id code
logger_id logger_create_id(char* p_sID) {
    return lgi_add_id(p_sID);
}

int logger_remove_id(logger_id id_ref) {
    if (id_ref == CLOGGER_DEFAULT_ID) {
        lgu_warn_msg("Can't remove the default logger_id.");
        return -1;
    }
    return lgi_remove_id(id_ref);
}

#ifndef NDEBUG
#include <unistd.h>

int logger_print_id(logger_id id_ref) {
    char dest[CLOGGER_ID_MAX_LEN];
    lgi_get_id(id_ref, dest);
    printf("The logger ID is: %s\n", dest);
    return 0;
}

bool logger_test_stop_logger() {

    // Tell the logger it's time to exit
    // This is only intended to be used with internal testing
    g_bExit = true;

    return true;
}

bool custom_test_logger_large_add_message() {

    printf("\n");


    int t_nBufferSize = logger_get_buffer_size();
    int t_nWarnBuffer = logger_get_buffer_close_warn();

    // the following must be true for the test to work
    if ((t_nBufferSize - (t_nWarnBuffer * 2)) <= 0) {
        printf("Cannot conduct test with current values\n");
        return false;
    }
    short t_nCurrentIndex = 0;

    // fill the buffer with (t_nBufferSize - t_nWarnBuffer) messages, and let them get read
    while (t_nCurrentIndex < (t_nBufferSize - t_nWarnBuffer)) {
        printf("the value of t_nCurrentIndex is: %d\n", t_nCurrentIndex);
        if (!logger_log_msg(LOGGER_INFO, (char*) "foobar")) {
            printf("Something went wrong before it should have\n");
        }
        t_nCurrentIndex++;
    }

    // Tell the logger to stop running
    if (!logger_test_stop_logger()) {
        printf("Failed to tell the logger to stop\n");
    }
    sleep(5);

    // When the index hits this number, we shouldn't be able to add more messages
    int t_nWarnIndex = t_nCurrentIndex - t_nWarnBuffer;
    int t_nMessagesToAdd = t_nWarnIndex + t_nWarnBuffer;

    for (short t_nCount = 0; t_nCount < t_nMessagesToAdd; t_nCount++) {
        if (!logger_log_msg(LOGGER_INFO, (char*) "here")) {
            printf("Failed too early\n");
            return false;
        }
    }

    // Now that we have reached the warn index, the next item should return false
    bool t_bFinalReturn = logger_log_msg(LOGGER_INFO, (char*) "fail here");
    if (t_bFinalReturn) {
        printf("This was supposed to be false.\n");
        return false;
    }
    else {
        printf("The test worked\n");
        return true;
    }

    logger_free();

}
#endif

