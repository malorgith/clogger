
#include "clogger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h> // sleep()

#ifdef __cplusplus
using namespace ::malorgith::clogger;
#endif  // __cplusplus

int main() {
    static char const* const kCloggerTestFilePathENV = "CLOGGER_TEST_FILE_PATH";
    static char const* const kCloggerTestFileNameENV = "CLOGGER_TEST_FILE_NAME";

    // get the log level by converting a string to an integer
    char const* const str_log_level = "debug";
    int int_log_level = logger_log_str_to_int(str_log_level);
    if (int_log_level < 0) {
        fprintf(stderr, "Failed to convert '%s' to an integer log level.\n", str_log_level);
        return 1;
    }

    // start the logger
    if (logger_init(int_log_level)) {
        fprintf(stderr, "Failed to initialize the logger.\n");
        return 1;
    }

    // create a console handler and store the reference to it
    loghandler_t stdout_handler = logger_create_console_handler(stdout);
    if (stdout_handler == kCloggerHandlerErr) {
        fprintf(stderr, "Failed to create the console handler.\n");
        return 1;
    };

    char const* const kCloggerMissingEnvMsg = "%s\n\tAn environment variable with name '%s' must be set to run this test\n";

    // create a file handler and store the reference to it
    char const* str_log_dir = getenv(kCloggerTestFilePathENV);
    char const* str_log_name = getenv(kCloggerTestFileNameENV);
    char const* str_file_test_disabled = "File test disabled";
    loghandler_t file_handler_ref = 0;
    if (str_log_dir == NULL) {
        printf(kCloggerMissingEnvMsg, str_file_test_disabled, kCloggerTestFilePathENV);
    } else if (str_log_name == NULL) {
        printf(kCloggerMissingEnvMsg, str_file_test_disabled, kCloggerTestFileNameENV);
    } else {
        file_handler_ref = logger_create_file_handler(str_log_dir, str_log_name);
        if (file_handler_ref == kCloggerHandlerErr) {
            fprintf(stderr, "Failed to create the file handler.\n");
            return 1;
        };
    }

    #ifdef CLOGGER_GRAYLOG
    static char const* kCloggerTestGraylogUrlENV = "CLOGGER_TEST_GRAYLOG_URL";
    static char const* kCloggerTestGraylogTcpPortENV = "CLOGGER_TEST_GRAYLOG_TCP_PORT";
    static char const* kCloggerTestGraylogUdpPortENV = "CLOGGER_TEST_GRAYLOG_PORT";

    // test graylog TCP connection
    {
        char const* str_graylog_url = getenv(kCloggerTestGraylogUrlENV);
        char const* str_graylog_port = getenv(kCloggerTestGraylogTcpPortENV);
        char const* kCloggerTestDisabledMsg = "Graylog TCP test disabled";
        loghandler_t t_refGraylogTcp = 0;
        if (str_graylog_port == NULL) {
            printf(kCloggerMissingEnvMsg, kCloggerTestDisabledMsg, kCloggerTestGraylogUdpPortENV);
        } else if (str_graylog_url == NULL) {
            printf(kCloggerMissingEnvMsg, kCloggerTestDisabledMsg, kCloggerTestGraylogUdpPortENV);
        } else {
            int port_num = atoi(str_graylog_port);
            if (port_num <= 0) {
                printf("Failed to convert '%s' to a valid integer to use as the Graylog port.\n", str_graylog_port);
            } else {
                t_refGraylogTcp = logger_create_graylog_handler(str_graylog_url, port_num, kGraylogTCP);
                if (t_refGraylogTcp == kCloggerHandlerErr) {
                    fprintf(stderr, "Failed to create the TCP Graylog handler!\n");
                    return 1;
                };
            }
        }
    }

    // test graylog UDP connection
    {
        char const* str_graylog_url = getenv(kCloggerTestGraylogUrlENV);
        char const* str_graylog_port = getenv(kCloggerTestGraylogUdpPortENV);
        char const* const kCloggerTestDisabledMsg = "Graylog UDP test disabled";
        loghandler_t t_refGraylogUdp = 0;
        if (str_graylog_url == NULL) {
            printf(kCloggerMissingEnvMsg, kCloggerTestDisabledMsg, kCloggerTestGraylogUdpPortENV);
        } else if (str_graylog_port == NULL) {
            printf(kCloggerMissingEnvMsg, kCloggerTestDisabledMsg, kCloggerTestGraylogUdpPortENV);
        }
        else {
            int port_num = atoi(str_graylog_port);
            if (port_num <= 0) {
                printf("Failed to convert '%s' to a valid integer to use as the Graylog port.\n", str_graylog_port);
            } else {
                t_refGraylogUdp = logger_create_graylog_handler(str_graylog_url, port_num, kGraylogUDP);
                if (t_refGraylogUdp == kCloggerHandlerErr) {
                    fprintf(stderr, "Failed to create the UDP Graylog handler!\n");
                    return 1;
                };
            }
        }
    }
    #endif  // CLOGGER_GRAYLOG

    #ifndef CLOGGER_NO_SLEEP
    // adjust the amount of time the logger sleeps between looking for messages on the buffer
    logger_set_sleep_time(300);
    #endif  // CLOGGER_NO_SLEEP

    logger_log_msg(kCloggerEmergency, "emergency message");
    logger_log_msg(kCloggerAlert, "alert message");
    logger_log_msg(kCloggerCritical, "critical message");
    logger_log_msg(kCloggerError, "error message");
    logger_log_msg(kCloggerWarn, "warn message");
    logger_log_msg(kCloggerNotice, "notice message");
    logger_log_msg(kCloggerInfo, "info message");
    logger_log_msg(kCloggerDebug, "debug message");

    // log the maximum message size
    logger_log_msg(kCloggerInfo, "The maximum message size is: %d", kCloggerMaxMessageSize);

    // log the buffer size
    logger_log_msg(kCloggerInfo, "The buffer size is: %d", kCloggerBufferSize);

    // log the maximum number of IDs
    logger_log_msg(kCloggerInfo, "The maximum number of IDs is: %d", kCloggerMaxNumIds);

    // log the maximum ID length
    logger_log_msg(kCloggerInfo, "The maximum ID length is: %d", kCloggerIdMaxLen);

    // log the maximum number of handlers
    logger_log_msg(kCloggerInfo, "The maximum number of handlers is: %d", kCloggerMaxNumHandlers);

    // create a logid_t to identify the calling thread/function
    logid_t log_id_test = logger_create_id("test_id");
    if (log_id_test == kCloggerMaxNumIds) {
        fprintf(stderr, "Failed to create the custom logid_t\n");
        logger_free();
        return 1;
    }

    // log a message to a specific ID
    if (logger_log_msg_id(kCloggerError, log_id_test, "test message with an id")) {
        fprintf(stderr, "Failed to log message to ID.\n");
        logger_free();
        return 1;
    }

    #ifdef CLOGGER_BUILD_MACROS
    logger_emergency(log_id_test, "emergency macro message");
    logger_alert(log_id_test, "alert macro message");
    logger_critical(log_id_test, "critical macro message");
    logger_error(log_id_test, "error macro message");
    logger_warn(log_id_test, "warn macro message");
    logger_notice(log_id_test, "notice macro message");
    logger_info(log_id_test, "info macro message");
    logger_debug(log_id_test, "debug macro message");
    #endif  // CLOGGER_BUILD_MACROS

    // remove a logid_t that isn't needed
    if (logger_remove_id(log_id_test)) {
        fprintf(stderr, "Failed to remove the custom logid_t.\n");
        logger_free();
        return 1;
    }

    // create a handler that goes to stderr
    loghandler_t stderr_ref = logger_create_console_handler(stderr);
    if (stderr_ref == kCloggerHandlerErr) {
        fprintf(stderr, "Failed to create stderr console handler.\n");
        logger_free();
        return 1;
    }

    // create a logid_t that goes to specific handlers
    loghandler_t stderr_and_file_handler = (loghandler_t)(stderr_ref | file_handler_ref);
    logid_t second_id = logger_create_id_w_handlers("test_w_handler", stderr_and_file_handler);
    if (second_id == kCloggerMaxNumIds) {
        fprintf(stderr, "Failed to create a second logid_t.\n");
        logger_free();
        return 1;
    }

    // log a message to a specific ID that has custom handler outputs
    logger_log_msg_id(kCloggerNotice, second_id, "message to specific ID and handlers");

    // update the log level
    if (logger_set_log_level(kCloggerInfo)) {
        fprintf(stderr, "Failed to update the log level.\n");
        logger_free();
        return 1;
    }

    // log a message that won't be sent due to log level
    if (logger_log_msg(kCloggerDebug, "message won't be logged")) {
        fprintf(stderr, "Failed to send message to logger.\n");
        logger_free();
        return 1;
    }

    // sleep to make sure all messages are processed
    sleep(3);

    // check if the logger is still running
    if (!logger_is_running()) {
        fprintf(stderr, "The logger stopped running.\n");
    }

    logger_log_msg(kCloggerAlert, "logger about to free");

    logger_free();
    return 0;
}

