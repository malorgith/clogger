
#include "handlers/console_handler.h"

#include <stdlib.h>
#include <string.h> //memcpy()

#include "logger_defines.h"

__MALORGITH_NAMESPACE_OPEN

// global variables
typedef struct {
    FILE* log_;
} consolehandler_data;

static const size_t kDataSize = { sizeof(consolehandler_data) };

// private function declarations
static int _console_handler_close(log_handler *handler_ptr);
static int _console_handler_isOpen(const log_handler* handler_ptr);
static int _console_handler_open(log_handler* handler_ptr);
static int _console_handler_write(log_handler *handler_ptr, const t_loggermsg* msg);

// private function definitions
int _console_handler_close(log_handler *handler_ptr) {

    if (lgh_checks(handler_ptr, kDataSize, "console handler"))
        return 1;

    ((consolehandler_data*)handler_ptr->handler_data_)->log_ = NULL;
    free(handler_ptr->handler_data_);
    handler_ptr->handler_data_ = NULL;
    return 0;
}

int _console_handler_isOpen(const log_handler *handler_ptr) {

    // return 1 so function passes if() checks
    if (lgh_checks(handler_ptr, kDataSize, "console handler"))
        return 0;
    else return 1;
}

int _console_handler_open(log_handler *handler_ptr) {

    if (lgh_checks(handler_ptr, kDataSize, "console handler"))
        return 1;

    return 0;
}

int _console_handler_write(log_handler *handler_ptr, const t_loggermsg* msg) {

    if (lgh_checks(handler_ptr, kDataSize, "console handler"))
        return 1;
    else if (msg == NULL) {
        lgu_warn_msg("console handler given NULL message to log");
        return 1;
    }

    fprintf(
        ((consolehandler_data*)handler_ptr->handler_data_)->log_,
        "%s%s %s\n",
        msg->format_,
        msg->id_,
        msg->msg_
    );
    fflush(((consolehandler_data*)handler_ptr->handler_data_)->log_);

    return 0;
}

// public function definitions
int create_console_handler(log_handler *handler_ptr, FILE *file_out) {

    if (file_out == NULL) {
        lgu_warn_msg("can't create console handler to NULL");
        return 1;
    }
    else if(handler_ptr == NULL) {
        lgu_warn_msg("can't store console handler in NULL ptr");
        return 1;
    }
    else if ((file_out != stdout ) && (file_out != stderr)) {
        lgu_warn_msg("console handler must be to stdout or stderr");
        return 1;
    }

    // allocate space for the handler data
    consolehandler_data* handler_data = (consolehandler_data*)malloc(sizeof(consolehandler_data));
    if (handler_data == NULL) {
        lgu_warn_msg("failed to allocate space for console handler data");
        return 1;
    }

    handler_data->log_ = file_out;

    log_handler t_structHandler = {
        &_console_handler_write,
        &_console_handler_close,
        &_console_handler_open,
        &_console_handler_isOpen,
        true,
        true,
        handler_data
    };

    memcpy(handler_ptr, &t_structHandler, sizeof(log_handler));

    return 0;
}

__MALORGITH_NAMESPACE_CLOSE
