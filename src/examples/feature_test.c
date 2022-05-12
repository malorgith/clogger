
// override the default buffer size by defining our own
#define CLOGGER_BUFFER_SIZE 100
// increase the size of the messages
#define LOGGER_MAX_MESSAGE_SIZE 500

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

    if (!logger_init(int_log_level)) {
        fprintf(stderr, "Failed to initialize the logger.\n");
        return 1;
    }

    // create the console handler and add it to the logger
    if (logger_create_console_handler(stdout) != 0) {
        fprintf(stderr, "Failed to create the console handler!\n");
        return 1;
    };

    // create the file handler and add it to the logger
    char* t_sPathName = getenv(_clogger_test_file_path_env);
    char* t_sFileName = getenv(_clogger_test_file_name_env);
    if (t_sPathName == NULL) {
        printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_file_path_env);
    }
    else if (t_sFileName == NULL) {
        printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_file_name_env);
    }
    else {
        if (logger_create_file_handler(t_sPathName, t_sFileName) != 0) {
            fprintf(stderr, "Failed to create the file handler!\n");
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
                if (logger_create_graylog_handler(t_sGraylogURL, t_nPort, GRAYLOG_TCP) != 0) {
                    fprintf(stderr, "Failed to create the file handler!\n");
                    return 1;
                };
            }
        }
    }

    // test graylog UDP connection
    {
        char* t_sGraylogURL = getenv(_clogger_test_graylog_url_env);
        char* t_sGraylogPort = getenv(_clogger_test_graylog_udp_port_env);
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
                if (logger_create_graylog_handler(t_sGraylogURL, t_nPort, GRAYLOG_UDP) != 0) {
                    fprintf(stderr, "Failed to create the file handler!\n");
                    return 1;
                };
            }
        }
    }
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

    logger_id log_id_test = logger_create_id("test_id");
    logger_log_msg_id(LOGGER_ERROR, log_id_test, "test message with an id");

    sleep(3);

    logger_log_msg(LOGGER_ALERT, "logger about to free\n");

    logger_free();
    return 0;
}
