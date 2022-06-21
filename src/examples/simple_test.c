
#include "clogger.h"

int main(__attribute__((unused))int argc, __attribute__((unused))char** argv) {
    // initialize logger and start logging thread
    if (logger_init(LOGGER_INFO)) {
        fprintf(stderr, "Failed to initialize logger.\n");
        return 1;
    }

    // create a file handler
    t_handlerref t_refFileHandler = logger_create_file_handler("./logs/", "simple_test.log");
    if (t_refFileHandler == CLOGGER_HANDLER_ERR) {
        fprintf(stderr, "Failed to open log file for writing.\n");
        logger_free();
        return 1;
    }

    // (optional) create a second handler that goes to stdout
    t_handlerref t_refStdout = logger_create_console_handler(stdout);
    if (t_refStdout == CLOGGER_HANDLER_ERR) {
        fprintf(stderr, "Failed to create stdout console handler.\n");
        logger_free();
        return 1;
    }

    // (optional) create a logger_id to identify the calling thread/function
    logger_id stdout_id = logger_create_id_w_handlers("stdout", t_refStdout);
    if (stdout_id == CLOGGER_MAX_NUM_IDS) {
        fprintf(stderr, "Failed to create stdout logger_id.\n");
        logger_free();
        return 1;
    }

    // log a message
    if(logger_log_msg(LOGGER_NOTICE, "Logging a message")) {
        fprintf(stderr, "Failed to add first message to logger.\n");
    }

    // log a message using a logger_id
    if (logger_log_msg_id(LOGGER_INFO, stdout_id, "Logging a message using a logger_id")) {
        fprintf(stderr, "Failed to add second message to logger.\n");
    }

    // stop the log thread, close open handlers, and free memory when done
    if (logger_free()) {
        fprintf(stderr, "Failed to stop the logger.\n");
    }

    return 0;
}

