
#include "clogger.h"

#ifdef __cplusplus
using namespace ::malorgith::clogger;
#endif  // __cplusplus

int main() {
    // initialize logger and start logging thread
    if (logger_init(kCloggerInfo)) {
        fprintf(stderr, "Failed to initialize logger.\n");
        return 1;
    }

    // create a file handler
    loghandler_t file_handler_ref = logger_create_file_handler("./logs/", "simple_test.log");
    if (file_handler_ref == kCloggerHandlerErr) {
        fprintf(stderr, "Failed to open log file for writing.\n");
        logger_free();
        return 1;
    }

    // (optional) create a second handler that goes to stdout
    loghandler_t stdout_handler_ref = logger_create_console_handler(stdout);
    if (stdout_handler_ref == kCloggerHandlerErr) {
        fprintf(stderr, "Failed to create stdout console handler.\n");
        logger_free();
        return 1;
    }

    // (optional) create a logid_t to identify the calling thread/function
    logid_t stdout_id = logger_create_id_w_handlers("stdout", stdout_handler_ref);
    if (stdout_id == kCloggerMaxNumIds) {
        fprintf(stderr, "Failed to create stdout logid_t.\n");
        logger_free();
        return 1;
    }

    // log a message
    if(logger_log_msg(kCloggerNotice, "Logging a message")) {
        fprintf(stderr, "Failed to add first message to logger.\n");
    }

    // log a message using a logid_t
    if (logger_log_msg_id(kCloggerInfo, stdout_id, "Logging a message using a logid_t")) {
        fprintf(stderr, "Failed to add second message to logger.\n");
    }

    // stop the log thread, close open handlers, and free memory when done
    if (logger_free()) {
        fprintf(stderr, "Failed to stop the logger.\n");
    }

    return 0;
}
