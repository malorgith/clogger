
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

// GLOBAL VARS
static atomic_bool g_bExit = { false };
static bool volatile g_logInit = { false };
static bool volatile g_bLoggerDebug = { false };
static int g_nLogLevel;
static int g_nHandlers = { 0 };        // the number of handlers stored in g_pHandlers
static pthread_t g_LogThread;
static log_handler* g_pHandlers[CLOGGER_MAX_NUM_HANDLERS];
static sem_t g_semModifyHandlers;      // controls access to handler variables

static logger_formatter* g_lgformatter = { NULL };

static int buf_refid = { -1 };

static bool volatile g_bTimestampEnabled = { true };

const logger_id CLOGGER_DEFAULT_ID = { 0 };
// END GLOBAL VARS

// PRIVATE FUNCTION DECLARATIONS
static int  _logger_add_handler(log_handler* p_pHandler);
static bool _logger_add_message(t_loggermsg* msg);
static bool _logger_log_msg(
    int log_level,
    logger_id id,
    char* format,
    char* msg,
    va_list arg_list
);
static bool _logger_read_message();
static void *_logger_run(void *p_pData);
static int _logger_timedwait(sem_t *p_pSem, int t_nWaitTimeSecs);
// END PRIVATE FUNCTION DECLARATIONS

// PRIVATE FUNCTION DEFINITIONS
bool _logger_add_message(t_loggermsg* msg) {

    if (!g_logInit) {
        free(msg);
        return false;
    }
    /*
     * Allow messages to be added to the buffer when there are no
     * handlers IF the logger has been initialized (above).
     */

    if (logger_buffer_add_message(buf_refid, msg)) {
        fprintf(stderr, "Logger failed to add message to buffer.\n");
        return false;
    }

    return true;

}

/*
 * Add a handler to the logger.
 */
int _logger_add_handler(log_handler* p_pHandler) {

    if (g_nHandlers >= CLOGGER_MAX_NUM_HANDLERS) {
        fprintf(stderr, "Error: maximum number of handlers already reached\n");
        return 1;
    }

    // TODO make global value or macro?
    int t_nSleepTimeMillisecs = 300;

    // wait for the semaphore to be available
    if (_logger_timedwait(&g_semModifyHandlers, t_nSleepTimeMillisecs) != 0) {
        // failed to get the semaphore or timed out; error should have already printed
        return 1;
    }

    // realloc() works as malloc() if the pointer is initially null

    // increment the count of handlers
    int t_nHandlers = g_nHandlers;
    t_nHandlers += 1;
    // resize the memory to create more space for the new handler
    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if (g_pHandlers[t_nCount] == NULL) {
            g_pHandlers[t_nCount] = (log_handler*)malloc(sizeof(log_handler));
            *g_pHandlers[t_nCount] = *(p_pHandler);
            break;
        }
    }
    g_nHandlers++;

    sem_post(&g_semModifyHandlers);

    return 0;
}

bool _logger_log_msg(
    int log_level,
    logger_id id,
    __attribute__((unused))char* format,
    char* msg,
    va_list arg_list
) {

    // TODO implement format parameter

    // Check the level
    if (log_level < 0) {
        fprintf(stderr, "Log level must be at least zero\n");
        return false;
    }
    else if (log_level > g_nLogLevel) {
        // This message won't be logged based on the log level
        return true; // return true because no error occurred
    }
    else if (!g_logInit) {
        return false;
    }

    t_loggermsg* t_sFinalMessage = (t_loggermsg*) malloc(sizeof(t_loggermsg));
    va_list arg_list_copy;
    va_copy(arg_list_copy, arg_list);
    int format_rtn = vsnprintf(t_sFinalMessage->m_sMsg, LOGGER_MAX_MESSAGE_SIZE, msg, arg_list_copy);
    va_end(arg_list_copy);
    if ((format_rtn >= LOGGER_MAX_MESSAGE_SIZE) || (format_rtn < 0)) {
        fprintf(stderr, "Failed to format the message before adding it to the buffer.\n");
        free(t_sFinalMessage);
        return false;
    }
    t_sFinalMessage->m_nLogLevel = log_level;
    t_sFinalMessage->m_nId = id;
    t_sFinalMessage->m_sFormat = NULL;

    // FIXME It's not ideal to get the logger ID here; there are locks around getting the value,
    // and it takes time to copy the result into the t_loggermsg struct. It would be better to
    // do this during _logger_read_message() since that won't impact the execution of the calling
    // thread, but the current issue is that the ID might be freed before its value can be copied
    // in _logger_read_message().
    char t_sId[LOGGER_ID_MAX_LEN];
    if (logger_id_get_id(t_sFinalMessage->m_nId, t_sId) != 0) {
        fprintf(stderr, "Failed to convert ID from reference to string.\n");
        // FIXME error?
    }
    int copy_rtn = snprintf(t_sFinalMessage->m_sId, LOGGER_ID_MAX_LEN * (sizeof(char)), "%s", t_sId);
    if ((copy_rtn >= LOGGER_ID_MAX_LEN) || (copy_rtn < 0)) {
        fprintf(stderr, "Failed to set the logger ID in the message.\n");
    }

    // TODO we should be able to disable getting the timestamp since it increases execution time, but
    // since we _might_ want to support handlers having their own logger_formatter objects, there would
    // be no way to know if any of the handlers' formatters require getting the time.
//  if (g_bTimestampEnabled) {
        time_t t_Time = time(NULL);
        struct tm t_TimeData;
        struct tm *t_pResult = localtime_r(&t_Time, &t_TimeData);
        if (t_pResult != &t_TimeData) {
            fprintf(stderr, "logger failed to get the time.\n");
            free(t_sFinalMessage);
            // TODO should this error?
            return 1;
        }
        t_sFinalMessage->m_tmTime = t_TimeData;
//  }

    return (_logger_add_message(t_sFinalMessage));
}

bool _logger_read_message() {

    if(!g_logInit)
        return false;
    else if (g_nHandlers < 1)
        return false;

    // wait for the handler semaphore to be available
    if (_logger_timedwait(&g_semModifyHandlers, 300) != 0) { // FIXME need to make this variable
        // failed to get the semaphore or timed out; error should have already printed
        return false;
    }

    // Get the message at the current index
    t_loggermsg* t_pMsg = logger_buffer_read_message(buf_refid);
    if (t_pMsg == NULL) {
        fprintf(stderr, "Failed to read a message from the buffer.\n");
        return false;
    }

    // get the format string
    char formatted_string[50]; // FIXME size
    if (lgf_format(g_lgformatter, formatted_string, &t_pMsg->m_tmTime, t_pMsg->m_nLogLevel)) {
        fprintf(stderr, "Failed to get the format for the message.\n");
        // FIXME does this abort function call?
    }
    t_pMsg->m_sFormat = formatted_string;

    /*
     * FIXME We used to get the actual value of the ID here, but there was an issue where a
     * message with an ID would come in, be placed on the buffer, then the ID would be removed.
     * We need a way to make sure IDs can't be removed until there are no pending messages that
     * contain that ID.
    // get the ID as a string
    char t_sId[LOGGER_ID_MAX_LEN];
    if (logger_id_get_id(t_pMsg->m_nId, t_sId) != 0) {
        fprintf(stderr, "Failed to convert ID from reference to string.\n");
        // FIXME error?
    }
    t_pMsg->m_sId = t_sId;
    */
    // now that we have the sempahore, write the message to each handler
    for (int t_nCount = 0; t_nCount < g_nHandlers; t_nCount++) {
        g_pHandlers[t_nCount]->write(t_pMsg);
    }
    sem_post(&g_semModifyHandlers); // let go of the semaphore
    free(t_pMsg);                   // free the memory

    return true;
}

void *_logger_run(__attribute__((unused))void *p_pData) {

    bool t_bExit = false;
    int t_nCurrentHandlers = 0;
    while(true) {

        short t_nMessagesBeforeCheck = 25;  // XXX This value should probably be less than BUFFER_SIZE
        short t_nMessagesRead = 0;

        while(t_nMessagesRead < t_nMessagesBeforeCheck) {
            // FIXME
            int wait_rtn = logger_buffer_wait_for_messages(buf_refid, 1); // FIXME need to make this variable
            // FIXME

            // check if any new handlers have been added
            if (t_nCurrentHandlers != g_nHandlers) {
                sem_wait(&g_semModifyHandlers);
                for (int count = 0; count < CLOGGER_MAX_NUM_HANDLERS; count++) {
                    if ((g_pHandlers[count] != NULL) && (!g_pHandlers[count]->isOpen())) {
                        if (!g_pHandlers[count]->open()) {
                            fprintf(stderr, "Logger thread failed to open a handler.\n");
                            // FIXME error handle
                            sem_post(&g_semModifyHandlers);
                            return NULL;
                        }
                    }
                }
                t_nCurrentHandlers = g_nHandlers;
                sem_post(&g_semModifyHandlers);
            }

            if (wait_rtn == 0) {
                // there's a message to read
//              printf("Found a message to read!\n");
                _logger_read_message();
                t_nMessagesRead++;
            }
            else if (wait_rtn > 0) {
                // timed out; break from the loop to check if we need to exit
                break;
            }
            else if (wait_rtn < 0) {
                // something went wrong
                fprintf(stderr, "Something went wrong while waiting for a message to be placed on the buffer.\n");
                // FIXME fail? error?
                break;
            }
        }

        if (t_bExit)
            // We've already detected that it's time to leave and gone through the loop an additional time
            break;

        // Check if it's time to exit
        if (g_bExit) {
            // time to go
            if (g_bLoggerDebug)
                printf("It's time to go.\n");
            // set t_bExit to true; we will loop one more time to get any remaining messages
            t_bExit = true;
        }
    }

    sem_wait(&g_semModifyHandlers);
    for (int count = 0; count < CLOGGER_MAX_NUM_HANDLERS; count++) {
        if ((g_pHandlers[count] != NULL) && (g_pHandlers[count]->isOpen())) {
            if (g_pHandlers[count]->close() != 0) {
                fprintf(stderr, "Logger thread failed to close a handler.\n");
                // FIXME error handle
                sem_post(&g_semModifyHandlers);
                return NULL;
            }
        }
    }
    sem_post(&g_semModifyHandlers);

    bool* rtn_val = (bool*) malloc(sizeof(bool));
    *rtn_val = true;
    pthread_exit(rtn_val);

}

int _logger_timedwait(sem_t *p_pSem, int milliseconds_to_wait) {

    static const int max_milliseconds_wait = 3000;
    static const int min_milliseconds_wait = 1;

    struct timespec t_timespecWait;
    if (clock_gettime(CLOCK_REALTIME, &t_timespecWait) == 1) {
        fprintf(stderr, "Failed to get the time before waiting for semaphore.");
        return 1;
    }

    logger_buffer_add_to_time(&t_timespecWait, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);

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
            fprintf(stderr, "Timed out trying to get semaphore.\n");
            return errno;
        }
    }

    return 0;
}
// END PRIVATE FUNCTION DEFINITIONS

bool logger_init(int p_nLogLevel) {

    if (g_logInit) {
        fprintf(stderr, "Logger has already been initialized\n");
        return false;
    }

    /*
    if (BUFFER_CLOSE_WARN > BUFFER_SIZE) {
        // invalid
        fprintf(stderr, "Buffer warning size can't be greater than buffer size\n");
        return false;
    }
    else if (BUFFER_CLOSE_WARN <= 0) {
        fprintf(stderr, "Buffer warning size must be greater than zero\n");
        return false;
    }
    else if ((BUFFER_SIZE - BUFFER_CLOSE_WARN) <= 0) {
        fprintf(stderr, "Buffer warning value must be below buffer size\n");
        return false;
    }
    */
    else if (! (LOGGER_SLEEP_SECS > 0)) {
        fprintf(stderr, "The amount of time the logger should sleep must be an integer greater than zero\n");
        return false;
    }

    if (logger_id_init() != 0) {
        fprintf(stderr, "Failed to initialize the logger IDs.\n");
        return false;
    }

    // create the default logger id, which is 'main'
    if (logger_create_id((char*) "main") < 0) {
        fprintf(stderr, "Failed to create default logger ID.\n");
        return false;
    }

    // create the formatter
    g_lgformatter = (logger_formatter*) malloc(sizeof(logger_formatter));
    if (g_lgformatter == NULL) {
        fprintf(stderr, "Failed to allocate space for the formatter.\n");
        return false;
    }
    if (lgf_init(g_lgformatter)) {
        fprintf(stderr, "Failed to initialize formatter.\n");
        return false;
    }
    lgf_set_datetime_format(g_lgformatter, "%Y-%m-%d %H:%M:%S");

    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        g_pHandlers[t_nCount] = NULL;
    }

    // Set the log level based on what the user specified
    if (p_nLogLevel < 0) {
        fprintf(stderr, "The log level must be at least zero.\n");
        return false;
    }
    g_nLogLevel = p_nLogLevel;

    // TODO make sure this value is >= 0 before changing it?
    buf_refid = logger_buffer_init();
    if (buf_refid < 0) {
        fprintf(stderr, "Failed to create the log buffer.\n");
        return false;
    }

    g_bExit = false;
    sem_init(&g_semModifyHandlers, 0, 1);

    // Start the log thread
    g_logInit = true;
    pthread_create(&g_LogThread, NULL, _logger_run, NULL);

#ifndef NDEBUG
    logger_log_msg(LOGGER_DEBUG, "Logger started with debugging built in");
#endif

    return true;
}

bool logger_free() {

    int t_bExitStatus = true;

    if (!g_logInit)
        return false;

    // Tell the logging thread it's time to end
    g_bExit = true;

    bool* join_val = NULL;
    pthread_join(g_LogThread, (void**) &join_val);
    if (join_val == NULL) {
        fprintf(stderr, "Failed to get status from log thread.\n");
        fflush(stderr);
    }
    else {
        if (*join_val != true) {
            fprintf(stderr, "Got non-true value on exit of log thread.\n");
            fflush(stderr);
            t_bExitStatus = false;
        }
        free(join_val);
    }

    g_logInit = false;
    if (logger_buffer_free()) {
        fprintf(stderr, "Failed to free the log buffer.\n");
        fflush(stderr);
        t_bExitStatus = false;
    }
    buf_refid = -1;

    // get the semaphore to modify the handlers
    if (_logger_timedwait(&g_semModifyHandlers, 300) != 0) {
        // failed to get the semaphore or timed out; error should have already printed
        // FIXME shouldn't just bomb out like this
        fprintf(stderr, "Failed to get the lock to modify the handlers.\n");
        fflush(stderr);
        t_bExitStatus = false;
    }

    // free the memory for the handlers; handlers are closed in _logger_run()
    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if (g_pHandlers[t_nCount] != NULL) {
            free(g_pHandlers[t_nCount]);
            g_pHandlers[t_nCount] = NULL;
        }
    }

    // free the formatters
    // FIXME semaphore(s) to modify values
    // TODO currently nothing to close(), but might change
    lgf_free(g_lgformatter);
    free(g_lgformatter);
    g_lgformatter = NULL;

    g_nHandlers = 0;
    sem_destroy(&g_semModifyHandlers);

    if (logger_id_free() != 0) {
        fprintf(stderr, "Failed to free the logger IDs.\n");
        t_bExitStatus = false;
    }

    return t_bExitStatus;
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
        fprintf(stderr, "Couldn't match specified level '%s' to a known log level.\n", p_sLogLevel);
        return -1;
    }
}

bool logger_is_running() {
    return g_logInit;
}

bool logger_log_msg(int p_nLogLevel, char* msg, ...) {
    va_list arg_list;
    va_start(arg_list, msg);
    bool rtn_val = _logger_log_msg(
        p_nLogLevel,
        0,   // ID
        NULL,   // format
        msg,
        arg_list
    );
    va_end(arg_list);

    return rtn_val;
}

bool logger_log_msg_id(int p_nLogLevel, logger_id log_id, char* msg, ...) {

    va_list arg_list;
    va_start(arg_list, msg);
    bool rtn_val = _logger_log_msg(
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
    return _logger_add_handler(&tmp_handler);
}

int logger_create_file_handler(char* p_sLogLocation, char* p_sLogName) {
    log_handler tmp_handler;
    int rtnval = create_file_handler(&tmp_handler, p_sLogLocation, p_sLogName);
    if (rtnval != 0) {
        return rtnval;
    }
    return _logger_add_handler(&tmp_handler);
}

#ifdef CLOGGER_GRAYLOG
int logger_create_graylog_handler(char* p_sServer, int p_nPort, int p_nProtocol) {
    log_handler tmp_handler;
    int rtnval = create_graylog_handler(&tmp_handler, p_sServer, p_nPort, p_nProtocol);
    if (rtnval != 0) {
        return rtnval;
    }
    return _logger_add_handler(&tmp_handler);
}
#endif

// id code

logger_id logger_create_id(char* p_sID) {
    return logger_id_add_id(p_sID);
}

#ifdef TESTING_FUNCTIONS_ENABLED
int logger_print_id(logger_id id_ref) {
    char dest[LOGGER_ID_MAX_LEN];
    logger_id_get_id(id_ref, dest);
    printf("The logger ID is: %s\n", dest);
    return 0;
}
#endif

int logger_remove_id(logger_id id_ref) {
    if (id_ref == CLOGGER_DEFAULT_ID) {
        fprintf(stderr, "Can't remove the default logger_id.\n");
        return -1;
    }
    return logger_id_remove_id(id_ref);
}

#ifdef TESTING_FUNCTIONS_ENABLED
#include <unistd.h>
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
//      if (!_logger_add_message("foobar")) {
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
//      if (!_logger_add_message("here")) {
        if (!logger_log_msg(LOGGER_INFO, (char*) "here")) {
            printf("Failed too early\n");
            return false;
        }
    }

    // Now that we have reached the warn index, the next item should return false
//  bool t_bFinalReturn = _logger_add_message("fail me");
    bool t_bFinalReturn = logger_log_msg(LOGGER_INFO, (char*) "fail me");
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

void logger_set_debug_output(bool p_bLogDebug) {
    g_bLoggerDebug = p_bLogDebug;
}

typedef struct {
    int m_nThreadNum;
    int m_nMessages;
} stressTestStruct;

bool logger_run_stress_test(int p_nThreads, int p_nMessages) {

    // now that handlers are a feature, we need to init the logger
    // and add the handlers before calling this
    if ((g_logInit != true) || (g_nHandlers <= 0)) {
        fprintf(stderr, "Must initialize the logger and add handlers before running the stress test.\n");
        return false;
    }

    printf("\nLogger Stress Test Starting\n");
    printf("===========================\n\n");

    pthread_t t_logTestThreads[p_nThreads];
    stressTestStruct t_Structs[p_nThreads];
    for (int t_nCount = 0; t_nCount < p_nThreads; t_nCount++) {

        stressTestStruct *my_struct = &t_Structs[t_nCount];
        my_struct->m_nThreadNum = t_nCount;
        my_struct->m_nMessages = p_nMessages;

        pthread_create(&t_logTestThreads[t_nCount], NULL, stress_test_logger, my_struct);
    }

    for (int t_nCount = 0; t_nCount < p_nThreads; t_nCount++) {
        pthread_join(t_logTestThreads[t_nCount], NULL);
    }
//  if (!logger_free()) return false;

    printf("\nLogger Stress Test Completed\n");
    printf("============================\n\n");

    return true;
}

void *stress_test_logger(void *p_pData) {

    stressTestStruct *my_params = (stressTestStruct*) p_pData;
    int t_nThread = (*my_params).m_nThreadNum;
    char tmp_id[50];
    logger_id id = 0;
    int snprintf_rtn = snprintf(tmp_id, 50, "Thread %d", t_nThread);
    if ((snprintf_rtn >= 50) || (snprintf_rtn < 0)) {
        fprintf(stderr, "Failed to create string for thread ID.\n");
    }
    else {
        id = logger_create_id(tmp_id);
    }
    int t_nMaxMessages = (*my_params).m_nMessages;

    for (int t_nCount = 0; t_nCount < t_nMaxMessages; t_nCount++) {
        char msg[80];
        sprintf(msg, "Message Number %d", t_nCount);
        // FIXME
        struct timespec t_sleeptime;
        // 2020103 changed this to decrease sleep time, needs to be fixed
        t_sleeptime.tv_nsec = (long) 10000000;
        t_sleeptime.tv_sec = 0;
        nanosleep(&t_sleeptime, NULL);
        // FIXME

//      if (!logger_log_msg(LOGGER_INFO, msg)) {
        if (!logger_log_msg_id(LOGGER_INFO, id, msg)) {
            printf("Thread %d: Got my first failure at message number %d\n", t_nThread, t_nCount);
            fflush(stdout);
        }
    }

    sleep(1);
    return NULL;
}
#endif

