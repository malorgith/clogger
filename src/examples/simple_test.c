
#include "clogger.h"

int main(__attribute__((unused))int argc, __attribute__((unused))char** argv) {
    // initialize logger and start logging thread
    if (logger_init(LOGGER_INFO)) {
        fprintf(stderr, "Failed to initialize logger.\n");
        return 1;
    }

    // create a file handler
    if (logger_create_file_handler("./logs/", "simple_test.log")) {
        fprintf(stderr, "Failed to open log file for writing.\n");
        logger_free();
        return 1;
    }

    // (optional) create a second handler that goes to stdout
    if (logger_create_console_handler(stdout)) {
        fprintf(stderr, "Failed to create console handler.\n");
        logger_free();
        return 1;
    }

    // (optional) create a logger_id to identify the calling thread/function
    logger_id log_id = logger_create_id("custom id");
    if (log_id < CLOGGER_DEFAULT_ID) {
        fprintf(stderr, "Failed to create logger_id.\n");
        logger_free();
        return 1;
    }

    // log a message
    if(logger_log_msg(LOGGER_NOTICE, "Logging a message")) {
        fprintf(stderr, "Failed to add first message to logger.\n");
    }

    // log a message using a logger_id
    if (logger_log_msg_id(LOGGER_INFO, log_id, "Logging a message using a logger_id")) {
        fprintf(stderr, "Failed to add second message to logger.\n");
    }

    // stop the log thread, close open handlers, and free memory when done
    if (logger_free()) {
        fprintf(stderr, "Failed to stop the logger.\n");
    }

    return 0;
}

