
// override the default buffer size by defining our own
#define CLOGGER_BUFFER_SIZE 100
// increase the size of the messages
#define CLOGGER_MAX_MESSAGE_SIZE 500

#include "clogger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h> // sleep()

int main(__attribute__((unused))int argc, __attribute__((unused))char** argv) {
    static const char* _clogger_test_file_path_env = "CLOGGER_TEST_FILE_PATH";
    static const char* _clogger_test_file_name_env = "CLOGGER_TEST_FILE_NAME";

    // get the log level by converting a string to an integer
    char* str_log_level = "debug";
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
    t_handlerref t_refStdout = logger_create_console_handler(stdout);
    if (t_refStdout == CLOGGER_HANDLER_ERR) {
        fprintf(stderr, "Failed to create the console handler.\n");
        return 1;
    };

    // create a file handler and store the reference to it
    char* t_sPathName = getenv(_clogger_test_file_path_env);
    char* t_sFileName = getenv(_clogger_test_file_name_env);
    t_handlerref t_refFile = 0;
    if (t_sPathName == NULL) {
        printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_file_path_env);
    }
    else if (t_sFileName == NULL) {
        printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_file_name_env);
    }
    else {
        t_refFile = logger_create_file_handler(t_sPathName, t_sFileName);
        if (t_refFile == CLOGGER_HANDLER_ERR) {
            fprintf(stderr, "Failed to create the file handler.\n");
            return 1;
        };
    }

#ifdef CLOGGER_GRAYLOG
    static const char* _clogger_test_graylog_url_env = "CLOGGER_TEST_GRAYLOG_URL";
    static const char* _clogger_test_graylog_tcp_port_env = "CLOGGER_TEST_GRAYLOG_TCP_PORT";
    static const char* _clogger_test_graylog_udp_port_env = "CLOGGER_TEST_GRAYLOG_PORT";

    // test graylog TCP connection
    {
        char* t_sGraylogURL = getenv(_clogger_test_graylog_url_env);
        char* t_sGraylogPort = getenv(_clogger_test_graylog_tcp_port_env);
        t_handlerref t_refGraylogTcp = 0;
        if (t_sGraylogURL == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_url_env);
        }
        else if (t_sGraylogPort == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_tcp_port_env);
        }
        else {
            int t_nPort = atoi(t_sGraylogPort);
            if (t_nPort <= 0) {
                printf("Failed to convert '%s' to a valid integer to use as the Graylog port.\n", t_sGraylogPort);
            }
            else {
                t_refGraylogTcp = logger_create_graylog_handler(t_sGraylogURL, t_nPort, GRAYLOG_TCP);
                if (t_refGraylogTcp == CLOGGER_HANDLER_ERR) {
                    fprintf(stderr, "Failed to create the TCP Graylog handler!\n");
                    return 1;
                };
            }
        }
    }

    // test graylog UDP connection
    {
        char* t_sGraylogURL = getenv(_clogger_test_graylog_url_env);
        char* t_sGraylogPort = getenv(_clogger_test_graylog_udp_port_env);
        t_handlerref t_refGraylogUdp = 0;
        if (t_sGraylogURL == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_url_env);
        }
        else if (t_sGraylogPort == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_udp_port_env);
        }
        else {
            int t_nPort = atoi(t_sGraylogPort);
            if (t_nPort <= 0) {
                printf("Failed to convert '%s' to a valid integer to use as the Graylog port.\n", t_sGraylogPort);
            }
            else {
                t_refGraylogUdp = logger_create_graylog_handler(t_sGraylogURL, t_nPort, GRAYLOG_UDP);
                if (t_refGraylogUdp == CLOGGER_HANDLER_ERR) {
                    fprintf(stderr, "Failed to create the UDP Graylog handler!\n");
                    return 1;
                };
            }
        }
    }
#endif

#ifndef CLOGGER_NO_SLEEP
    // adjust the amount of time the logger sleeps between looking for messages on the buffer
    logger_set_sleep_time(300);
#endif

    logger_log_msg(LOGGER_NOTICE, "first message from main");

    logger_log_msg(LOGGER_EMERGENCY, "This emergency message");
    logger_log_msg(LOGGER_ALERT, "This alert message");
    logger_log_msg(LOGGER_CRITICAL, "This critical message");
    logger_log_msg(LOGGER_ERROR, "This error message");
    logger_log_msg(LOGGER_WARN, "This warn message");
    logger_log_msg(LOGGER_NOTICE, "This notice message");
    logger_log_msg(LOGGER_INFO, "This info message");
    logger_log_msg(LOGGER_DEBUG, "This debug message");

    // create a logger_id to identify the calling thread/function
    logger_id log_id_test = logger_create_id("test_id");
    if (log_id_test == CLOGGER_MAX_NUM_IDS) {
        fprintf(stderr, "Failed to create the custom logger_id\n");
        return 1;
    }

    // log a message to a specific ID
    logger_log_msg_id(LOGGER_ERROR, log_id_test, "test message with an id");

    // remove a logger_id that isn't needed
    if (logger_remove_id(log_id_test)) {
        fprintf(stderr, "Failed to remove the custom logger_id.\n");
        return 1;
    }

    // create a handler that goes to stderr
    t_handlerref t_refStderr = logger_create_console_handler(stderr);
    if (t_refStderr == CLOGGER_HANDLER_ERR) {
        fprintf(stderr, "Failed to create stderr console handler.\n");
        logger_free();
        return 1;
    }

    // create a logger_id that goes to specific handlers
    logger_id second_id = logger_create_id_w_handlers("test_w_handler", (t_refStderr | t_refFile));
    if (second_id == CLOGGER_MAX_NUM_IDS) {
        fprintf(stderr, "Failed to create a second logger_id.\n");
        logger_free();
        return 1;
    }

    // log a message to a specific ID that has custom handler outputs
    logger_log_msg_id(LOGGER_NOTICE, second_id, "message to specific ID and handlers");

    sleep(3);

    // check if the logger is still running
    if (!logger_is_running()) {
        fprintf(stderr, "The logger stopped running.\n");
    }

    logger_log_msg(LOGGER_ALERT, "logger about to free");

    logger_free();
    return 0;
}

