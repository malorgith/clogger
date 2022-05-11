
#include "clogger.h"

#include "logger.h" // test functions

#include <stdlib.h>
#include <string.h>
#include <unistd.h> // sleep()

#define CLOGGER_TEST_FILE
//#define CLOGGER_TEST_GRAYLOG_TCP
#define CLOGGER_TEST_GRAYLOG_UDP

#if defined CLOGGER_TEST_GRAYLOG_TCP && defined CLOGGER_TEST_GRAYLOG_UDP
#warning "The same URL and port will be used to test TCP and UDP connections to Graylog"
#endif

int main(__attribute__((unused))int argc, __attribute__((unused))char** argv) {
    static const char* _clogger_test_file_path_env = "CLOGGER_TEST_FILE_PATH";
    static const char* _clogger_test_file_name_env = "CLOGGER_TEST_FILE_NAME";

    static const char* _clogger_test_graylog_url_env = "CLOGGER_TEST_GRAYLOG_URL";
    static const char* _clogger_test_graylog_port_env = "CLOGGER_TEST_GRAYLOG_PORT";

    if (!logger_init(LOGGER_DEBUG)) {
        fprintf(stderr, "Failed to initialize the logger.\n");
        return 1;
    }

    // create the console handler and add it to the logger
    if (logger_create_console_handler(stdout) != 0) {
        fprintf(stderr, "Failed to create the console handler!\n");
        return 1;
    };

    // create the file handler and add it to the logger
#ifdef CLOGGER_TEST_FILE
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
#endif

    // create the Graylog handler and add it to the logger
#ifdef CLOGGER_TEST_GRAYLOG_TCP
    {
        char* t_sGraylogURL = getenv(_clogger_test_graylog_url_env);
        char* t_sGraylogPort = getenv(_clogger_test_graylog_port_env);
        if (t_sGraylogURL == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_url_env);
        }
        else if (t_sGraylogPort == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_port_env);
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
#endif

#ifdef CLOGGER_TEST_GRAYLOG_UDP
    {
        char* t_sGraylogURL = getenv(_clogger_test_graylog_url_env);
        char* t_sGraylogPort = getenv(_clogger_test_graylog_port_env);
        if (t_sGraylogURL == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_url_env);
        }
        else if (t_sGraylogPort == NULL) {
            printf("An environment variable with name '%s' must be set to run this test\n", _clogger_test_graylog_port_env);
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

    logger_id my_id_foobar = logger_create_id("FOOBAR");
    logger_print_id(my_id_foobar);
    logger_id my_id_foo = logger_create_id("foo");
    logger_print_id(my_id_foo);
    logger_id my_id_bar = logger_create_id("");
    logger_print_id(my_id_bar);
    logger_log_msg_id(LOGGER_ERROR, my_id_foo, "this message at %d", 42);

#ifdef CLOGGER_STRESS_TEST
    logger_run_stress_test(5, 25);
#endif

    sleep(10);

    logger_log_msg(LOGGER_ALERT, "logger about to free\n");

    logger_free();
    return 0;
}
