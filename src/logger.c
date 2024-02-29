
#include "logger.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>     // for strcmp
#ifndef NDEBUG
#include <unistd.h>
#endif  // NDEBUG

#include "handlers/console_handler.h"
#include "handlers/file_handler.h"
#include "logger_atomic.h"
#include "logger_buffer.h"
#include "logger_defines.h"
#include "logger_formatter.h"
#ifdef CLOGGER_GRAYLOG
#include "handlers/graylog_handler.h"
#endif  // CLOGGER_GRAYLOG
#include __MALORGITH_CLOGGER_INCLUDE

#define LOGGER_SLEEP_SECS 1

__MALORGITH_NAMESPACE_OPEN
#ifdef __cplusplus
namespace {
#endif  // __cplusplus

// global variables
static bool volatile global_logger_running = { false };
static int global_log_level = { LOGGER_INFO };
#ifndef CLOGGER_NO_SLEEP
static unsigned int const global_max_sleep_time = { 5000 };
#endif  // CLOGGER_NO_SLEEP
static pthread_t *global_log_thread = { NULL };

static MALORGITH_CLOGGER_ATOMIC_BOOL global_exit = { false };
static MALORGITH_CLOGGER_ATOMIC_BOOL global_err_exit = { false };
//static MALORGITH_CLOGGER_ATOMIC_INT global_sleep_time = { 1 };
static MalorgithAtomicInt global_sleep_time = { 1 };

static logger_formatter* global_log_formatter = { NULL };

static int buf_refid = { -1 };

// TODO value below should be removed or determined based on what the handlers require
static bool volatile global_timestamp_enabled = { true };

/*
typedef struct {
    bool global_logger_running;
    int global_log_level;
    const unsigned int global_max_sleep_time;
    pthread_t* global_log_thread;

    MALORGITH_CLOGGER_ATOMIC_BOOL global_exit;
    MALORGITH_CLOGGER_ATOMIC_INT global_sleep_time;

    logger_formatter* global_log_formatter;

    int buf_refid;

    bool volatile global_timestamp_enabled;
} logger_global_data;

static logger_global_data globals = {
    false,
    -1,
    5000,
    NULL,
    false,
    1,
    NULL,
    -1,
    true
};
*/

// private function declarations
static int _logger_read_message();
static void *_logger_run(void *thread_data);

int _logger_read_message() {
    if(!global_logger_running) {
        return 1;
    } else if (lgh_get_num_handlers() < 1) {
        return 1;
    }

    // Get the message at the current index
    t_loggermsg* msg = lgb_read_message(buf_refid);
    if (msg == NULL) {
        lgu_warn_msg("failed to read a message from the buffer");
        return 1;
    }

    struct tm time_data;
    if (global_timestamp_enabled) {
        time_t timestamp = msg->timestamp_;
        struct tm *format_result = localtime_r(&timestamp, &time_data);
        if (format_result != &time_data) {
            lgu_warn_msg("logger failed to get the time");
            return 1;
        }
    }

    // get the format string
    char formatted_string[50]; // FIXME size
    if (lgf_format(global_log_formatter, formatted_string, &time_data, msg->log_level_)) {
        lgu_warn_msg("failed to get the format for the message");
        return 1;
    }
    msg->format_ = formatted_string;

    loghandler_t t_refHandlers = lgi_get_id(msg->ref_id_, msg->id_);
    if (t_refHandlers == 0) {
        lgu_warn_msg("failed to convert ID from reference to string");
        lgi_remove_message(msg->ref_id_);
        return 1;
    }
    lgi_remove_message(msg->ref_id_);

    // write to the handlers specified by the message
    if (lgh_write(t_refHandlers, msg)) {
        // we either failed to write to one or more handlers, or there were no open
        // handlers to write to
        lgu_warn_msg("logger thread failed to write to a handler");
    }
    return 0;
}

void *_logger_run(__attribute__((unused))void *thread_data) {
    bool t_bExit = false;
    while(true) {

        short t_nMessagesBeforeCheck = 25;  // TODO This value should probably be less than the buffer size
        short t_nMessagesRead = 0;

        while(t_nMessagesRead < t_nMessagesBeforeCheck) {
            int wait_rtn = lgb_wait_for_messages(buf_refid, global_sleep_time);

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
                lgu_warn_msg("something went wrong while waiting for a message to be placed on the buffer");
                // FIXME fail? error?
                break;
            }
        }

        if (t_bExit)
            // We've already detected that it's time to leave and gone through the loop an additional time
            break;

        // Check if it's time to exit
        if (global_exit) {
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

#ifdef __cplusplus
}  // namespace
#endif  // __cplusplus

const logid_t kCloggerDefaultId = { 0 };

// private function definitions
int _logger_log_msg(
    int log_level,
    logid_t id,
    __attribute__((unused))char* format,  // TODO(malorgith): implement
    char const* msg,
    va_list arg_list
) {

    // Check the level
    if (log_level < 0) {
        lgu_warn_msg("log level must be at least zero");
        return 1;
    }
    else if (log_level > global_log_level) {
        // This message won't be logged based on the log level
        return 0;  // return 0 because no error occurred
    }
    else if (!global_logger_running) {
        lgu_warn_msg("can't add message; logger isn't running");
        return 1;
    }

    // store the formatted message locally to ensure it doesn't exceed the size
    char tmp_msg[CLOGGER_MAX_MESSAGE_SIZE];
    va_list arg_list_copy;
    va_copy(arg_list_copy, arg_list);
    int format_rtn = vsnprintf(tmp_msg, CLOGGER_MAX_MESSAGE_SIZE, msg, arg_list_copy);
    va_end(arg_list_copy);
    if ((format_rtn >= CLOGGER_MAX_MESSAGE_SIZE) || (format_rtn < 0)) {
        lgu_warn_msg("failed to format the message before adding it to the buffer");
        return 1;
    }

    if (lgi_add_message(id)) {
        // failed to indicate there's a message waiting to use the ID
        lgu_warn_msg("can't add message to buffer because the ID couldn't be incremented");
        return 1;
    }

    t_loggermsg local_msg;
    memcpy(local_msg.msg_, tmp_msg, sizeof(char) * CLOGGER_MAX_MESSAGE_SIZE);
    local_msg.log_level_ = log_level;
    local_msg.format_ = NULL;
    local_msg.ref_id_ = id;

    /*
     * TODO
     * Add support for querying all set handlers to see if any require a timestamp.
     * If no handlers need one, then we don't need to set the data in the message struct.
     * Right now we assume at least one handler requires a timestamp.
     */

    if (global_timestamp_enabled) {
        local_msg.timestamp_ = time(NULL);
    }

    if (lgb_add_message(buf_refid, &local_msg)) {
        lgu_warn_msg("logger failed to add message to buffer");
        // update message count on id since message wasn't added
        lgi_remove_message(local_msg.ref_id_);
        return 1;
    }
    return 0;
}

// public functions
int logger_init(int log_level) {
    #ifndef NDEBUG
    #ifndef CLOGGER_REMOVE_WARNING
    fprintf(stderr, "\n(clogger) WARNING: library not compiled as release build; compile a release build to remove this warning\n\n");
    #endif  // CLOGGER_REMOVE_WARNING
    #endif  // NDEBUG

    if (global_logger_running) {
        lgu_warn_msg("logger has already been initialized");
        return 1;
    }
    else if (global_log_thread != NULL) {
        lgu_warn_msg("logger thread isn't null");
        return 1;
    }
    else if (log_level < 0) {
        lgu_warn_msg("the log level must be at least zero");
        return 1;
    }
    else if (log_level > LOGGER_MAX_LEVEL) {
        lgu_warn_msg_int("the log level must be no greater than %d", LOGGER_MAX_LEVEL);
        return 1;
    }
    else if (! (LOGGER_SLEEP_SECS > 0)) {
        lgu_warn_msg("the amount of time the logger should sleep must be an integer greater than zero");
        return 1;
    }

    if (lgi_init() != 0) {
        lgu_warn_msg("failed to initialize the logger IDs");
        global_err_exit = true;
        return logger_free();
    }

    // init the handler
    if (lgh_init()) {
        global_err_exit = true;
        return logger_free();
    }

    // create the default logger id, which is 'main'; must do after lgh_init()
    if (logger_create_id("main") == CLOGGER_MAX_NUM_IDS) {
        lgu_warn_msg("failed to create default logger ID");
        global_err_exit = true;
        return logger_free();
    }

    // create the formatter
    global_log_formatter = (logger_formatter*) malloc(sizeof(logger_formatter));
    if (global_log_formatter == NULL) {
        lgu_warn_msg("failed to allocate space for the formatter");
        global_err_exit = true;
        return logger_free();
    }
    if (lgf_init(global_log_formatter)) {
        lgu_warn_msg("failed to initialize formatter");
        global_err_exit = true;
        return logger_free();
    }
    lgf_set_datetime_format(global_log_formatter, "%Y-%m-%d %H:%M:%S");

    // Set the log level based on what the user specified
    global_log_level = log_level;

    // TODO make sure this value is >= 0 before changing it?
    buf_refid = lgb_init();
    if (buf_refid < 0) {
        lgu_warn_msg("failed to create the log buffer");
        global_err_exit = true;
        return logger_free();
    }

    global_exit = false;

    // Start the log thread
    global_logger_running = true;
    global_log_thread = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_create(global_log_thread, NULL, _logger_run, NULL);

#ifndef NDEBUG
#ifndef CLOGGER_REMOVE_WARNING
    logger_log_msg(LOGGER_WARN, "Logger started with debugging built in");
#endif  // CLOGGER_REMOVE_WARNING
#endif  // NDEBUG

    return 0;
}

int logger_free() {
    int return_code = 0;

    if (!global_logger_running && !global_err_exit)
        return 1;

    // Tell the logging thread it's time to end
    global_exit = true;

    if (global_log_thread) {
        bool* join_val = NULL;
        pthread_join(*global_log_thread, (void**) &join_val);
        if (join_val == NULL) {
            lgu_warn_msg("failed to get status from log thread");
            fflush(stderr);
        }
        else {
            if (*join_val != true) {
                lgu_warn_msg("got non-true value on exit of log thread");
                fflush(stderr);
                return_code = 1;
            }
            free(join_val);
        }
        free(global_log_thread);
        global_log_thread = NULL;
    }

    global_logger_running = false;

    if (lgi_free() != 0) {
        lgu_warn_msg("failed to free the logger IDs");
        return_code = 1;
    }

    if (lgb_free()) {
        lgu_warn_msg("failed to free the log buffer");
        fflush(stderr);
        return_code = 1;
    }
    buf_refid = -1;

    // free the handler memory
    if (lgh_free()) {
        return_code = 1;
    }

    // free the formatters
    // FIXME semaphore(s) to modify values
    // TODO currently nothing to close(), but might change
    lgf_free(global_log_formatter);
    free(global_log_formatter);
    global_log_formatter = NULL;

    if (global_err_exit) {
        return_code = 1;
        global_err_exit = false;
    }

    return return_code;
}

int logger_log_str_to_int(char const* str_log_level) {
    if (strcmp(str_log_level, "emergency") == 0) {
        return LOGGER_EMERGENCY;
    }
    else if (strcmp(str_log_level, "alert") == 0) {
        return LOGGER_ALERT;
    }
    else if (strcmp(str_log_level, "critical") == 0) {
        return LOGGER_CRITICAL;
    }
    else if (strcmp(str_log_level, "error") == 0) {
        return LOGGER_ERROR;
    }
    else if(strcmp(str_log_level, "warn") == 0) {
        return LOGGER_WARN;
    }
    else if (strcmp(str_log_level, "notice") == 0) {
        return LOGGER_NOTICE;
    }
    else if (strcmp(str_log_level, "info") == 0) {
        return LOGGER_INFO;
    }
    else if (strcmp(str_log_level, "debug") == 0) {
        return LOGGER_DEBUG;
    }
    else {
        #ifdef CLOGGER_VERBOSE_WARNING
        int msg_max_size = 300;
        char str_log_msg[msg_max_size];
        char const* str_err_msg = "Couldn't match specified level '%s' to a known log level";
        int format_rtn = snprintf(str_log_msg, msg_max_size, str_err_msg, str_log_level);
        if ((format_rtn == 0) || (format_rtn >= msg_max_size)) {
            // failed to format the message
            lgu_warn_msg("couldn't match given string to a known log level");
        }
        else {
            lgu_warn_msg(str_err_msg);
        }
        #endif // CLOGGER_VERBOSE_WARNING
        return -1;
    }
}

int logger_is_running() {
    if (global_logger_running) return 1;
    else return 0;
}

int logger_log_msg(int log_level, char const* msg, ...) {
    va_list arg_list;
    va_start(arg_list, msg);
    int rtn_val = _logger_log_msg(
        log_level,
        0,   // ID
        NULL,   // format
        msg,
        arg_list
    );
    va_end(arg_list);

    return rtn_val;
}

int logger_log_msg_id(int log_level, logid_t log_id, char const* msg, ...) {

    va_list arg_list;
    va_start(arg_list, msg);
    int rtn_val = _logger_log_msg(
        log_level,
        log_id,   // ID
        NULL,   // format
        msg,
        arg_list
    );
    va_end(arg_list);

    return rtn_val;
}

int logger_get_log_level() {
    return global_log_level;
}

int logger_set_log_level(int log_level) {
    if (log_level < 0) {
        lgu_warn_msg("Log level must be zero or greater.");
        return 1;
    }
    else if (log_level > LOGGER_MAX_LEVEL) {
        lgu_warn_msg_int("Log level must be less than or equal to %d", LOGGER_MAX_LEVEL);
        return 1;
    }
    global_log_level = log_level;
    return 0;
}

// handler code
loghandler_t logger_create_console_handler(FILE *out_file) {
    log_handler tmp_handler;
    if (create_console_handler(&tmp_handler, out_file))
        return kCloggerHandlerErr;

    loghandler_t t_refHandler = lgh_add_handler(&tmp_handler);
    if (t_refHandler == kCloggerHandlerErr) {
        return kCloggerHandlerErr;
    }

    // TODO for now we assume we want to add all handlers to the main id
    if (lgi_add_handler(kCloggerDefaultId, t_refHandler)) {
        lgu_warn_msg("failed to add new console handler to main id");
        // TODO didn't actually fail to create handler...
        return kCloggerHandlerErr;
    }

    return t_refHandler;
}

loghandler_t logger_create_file_handler(char const* str_log_dir, char const* str_log_name) {
    log_handler tmp_handler;
    if (create_file_handler(&tmp_handler, str_log_dir, str_log_name))
        return kCloggerHandlerErr;

    loghandler_t t_refHandler = lgh_add_handler(&tmp_handler);
    if (t_refHandler == kCloggerHandlerErr) {
        return kCloggerHandlerErr;
    }

    // TODO for now we assume we want to add all handlers to the main id
    if (lgi_add_handler(kCloggerDefaultId, t_refHandler)) {
        lgu_warn_msg("failed to add new file handler to main id");
        // TODO didn't actually fail to create handler...
        return kCloggerHandlerErr;
    }

    return t_refHandler;
}

// id code
logid_t logger_create_id(char const* str_log_id) {
    logid_t t_refId = lgi_add_id(str_log_id);
    if (t_refId == CLOGGER_MAX_NUM_IDS)
        return CLOGGER_MAX_NUM_IDS;

    // TODO for this implementation, we will assume the id gets all available handlers
    loghandler_t t_refAllHandlers = lgh_get_valid_handlers();
    if (lgi_add_handler(t_refId, t_refAllHandlers)) {
        lgu_warn_msg("failed to add all handlers to newly created id");
        return CLOGGER_MAX_NUM_IDS;
    }

    return t_refId;
}

logid_t logger_create_id_w_handlers(
    char const* str_log_id,
    loghandler_t log_handlers
) {
    // TODO we could verify that the handlers we were given are valid...
    return lgi_add_id_w_handler(str_log_id, log_handlers);
}

int logger_remove_id(logid_t id_ref) {
    if (id_ref == kCloggerDefaultId) {
        lgu_warn_msg("can't remove the default logid_t");
        return 1;
    }
    return lgi_remove_id(id_ref);
}

#ifndef NDEBUG
int logger_print_id(logid_t id_ref) {
    char dest[CLOGGER_ID_MAX_LEN];
    lgi_get_id(id_ref, dest);
    printf("The logger ID is: %s\n", dest);
    return 0;
}

bool logger_test_stop_logger() {
    // Tell the logger it's time to exit
    // This is only intended to be used with internal testing
    global_exit = true;

    return true;
}

bool custom_test_logger_large_add_message() {
    printf("\n");


    int buffer_size = logger_get_buffer_size();
    int warn_buffer_value = logger_get_buffer_close_warn();

    // the following must be true for the test to work
    if ((buffer_size - (warn_buffer_value * 2)) <= 0) {
        printf("Cannot conduct test with current values\n");
        return false;
    }
    short current_index = 0;

    // fill the buffer with (buffer_size - warn_buffer_value) messages, and let them get read
    while (current_index < (buffer_size - warn_buffer_value)) {
        printf("the value of current_index is: %d\n", current_index);
        if (!logger_log_msg(LOGGER_INFO, "foobar")) {
            printf("Something went wrong before it should have\n");
        }
        current_index++;
    }

    // Tell the logger to stop running
    if (!logger_test_stop_logger()) {
        printf("Failed to tell the logger to stop\n");
    }
    sleep(5);

    // When the index hits this number, we shouldn't be able to add more messages
    int warn_index = current_index - warn_buffer_value;
    int messages_to_add = warn_index + warn_buffer_value;

    for (short count = 0; count < messages_to_add; count++) {
        if (!logger_log_msg(LOGGER_INFO, "here")) {
            printf("Failed too early\n");
            return false;
        }
    }

    // Now that we have reached the warn index, the next item should return false
    bool final_return = logger_log_msg(LOGGER_INFO, "fail here");
    if (final_return) {
        printf("This was supposed to be false.\n");
        return false;
    }
    else {
        printf("The test worked\n");
        return true;
    }

    logger_free();
}
#endif  // NDEBUG

#ifdef CLOGGER_GRAYLOG
loghandler_t logger_create_graylog_handler(char const* graylog_url, int port, int protocol) {
    log_handler tmp_handler;
    if (create_graylog_handler(&tmp_handler, graylog_url, port, protocol))
        return kCloggerHandlerErr;

    loghandler_t t_refHandler = lgh_add_handler(&tmp_handler);
    if (t_refHandler == kCloggerHandlerErr) {
        return kCloggerHandlerErr;
    }

    // TODO for now we assume we want to add all handlers to the main id
    if (lgi_add_handler(kCloggerDefaultId, t_refHandler)) {
        lgu_warn_msg("failed to add new Graylog handler to main id");
        // TODO didn't actually fail to create handler...
        return kCloggerHandlerErr;
    }

    return t_refHandler;
}
#endif  // CLOGGER_GRAYLOG

#ifndef CLOGGER_NO_SLEEP
int logger_set_sleep_time(unsigned int milliseconds_to_sleep) {
    if (milliseconds_to_sleep > global_max_sleep_time) {
        lgu_warn_msg_int("Max sleep time cannot exceed %d milliseconds", (int)global_max_sleep_time);
        return 1;
    }
    global_sleep_time = milliseconds_to_sleep;
    return 0;
}
#endif // CLOGGER_NO_SLEEP

__MALORGITH_NAMESPACE_CLOSE
