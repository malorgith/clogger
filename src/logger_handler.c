
#include "logger_handler.h"

#include <stdlib.h>
#include <semaphore.h>
#include <string.h>  //memset()

//#include "clogger.h"
#include "logger_atomic.h"
#include "logger_defines.h"
#include "logger_msg.h"
#include __MALORGITH_CLOGGER_INCLUDE

__MALORGITH_NAMESPACE_OPEN
#ifdef __cplusplus
namespace {
#endif // __cplusplus

// file global vars
static log_handler** global_handler_storage = { NULL };
static log_handler** global_last_handler = { NULL };
static sem_t* global_lock = { NULL };
static MALORGITH_CLOGGER_ATOMIC_BOOL global_handler_init = { false };
static MALORGITH_CLOGGER_ATOMIC_INT  global_num_handlers = { 0 };

static loghandler_t global_valid_handler_refs = { 0 };

// private function declarations
int _lgh_check_init();

// private function definitions
int _lgh_check_init() {
    char const* str_err_msg = NULL;
    if (!global_handler_init) {
        str_err_msg = "handlers not initialized";
    }
    else if (global_handler_storage == NULL) {
        str_err_msg = "handlers not allocated";
    }
    else if (global_lock == NULL) {
        str_err_msg = "handler storage lock doesn't exist";
    }

    if (str_err_msg != NULL) {
        lgu_warn_msg(str_err_msg);
        return 1;
    }
    return 0;
}

#ifdef __cplusplus
}  // namespace
#endif  // __cplusplus

const loghandler_t kCloggerHandlerErr = { (1<<1) | (1<<2) };

// public functions
int lgh_init() {

    static MALORGITH_CLOGGER_ATOMIC_BOOL init_running = { false };
    if (init_running) {
        return 1;
    }
    init_running = true;

    if ((global_handler_storage != NULL) || (global_lock != NULL)) {
        lgu_warn_msg("handlers have already been initialized");
        init_running = false;
        return 1;
    }

    // allocate space for the handlers
    global_handler_storage = (log_handler**)malloc(sizeof(log_handler*) * CLOGGER_MAX_NUM_HANDLERS);
    if (global_handler_storage == NULL) {
        lgu_warn_msg("failed to allocate space for the handlers");
        init_running = false;
        return 1;
    }
    global_last_handler = &global_handler_storage[CLOGGER_MAX_NUM_HANDLERS - 1];

    for (
        log_handler** handler_ptr = global_handler_storage;
        handler_ptr < global_last_handler;
        ++handler_ptr
    ) {
        *handler_ptr = NULL;
    }

    // this should already be set
    global_valid_handler_refs = 0;

    // allocate space for the semaphore
    global_lock = (sem_t*)malloc(sizeof(sem_t));
    if (global_lock == NULL) {
        lgu_warn_msg("failed to allocate space for the handler lock");
        init_running = false;
        return 1;
    }
    global_num_handlers = 0;
    sem_init(global_lock, 0, 1);

    global_handler_init = true;

    init_running = false;
    return 0;
}

int lgh_free() {

    global_handler_init = false;

    int rtn_val = 0;
    if (global_lock == NULL) {
        lgu_warn_msg("handler lock doesn't exist");
        ++rtn_val;
    }
    else {
        // TODO might not need this if we implement individual handler free function
        sem_wait(global_lock);
    }

    if (global_handler_storage == NULL) {
        lgu_warn_msg("no handlers to free");
        ++rtn_val;
    }
    else {
        for (
            log_handler** handler_ptr = global_handler_storage;
            handler_ptr < global_last_handler;
            ++handler_ptr
        ) {
            if (*handler_ptr != NULL) {
                free(*handler_ptr);
            }
        }
        free(global_handler_storage);
        global_handler_storage = NULL;
    }

    global_num_handlers = 0;
    global_valid_handler_refs = 0;
    global_last_handler = NULL;

    sem_destroy(global_lock);
    free(global_lock);
    global_lock = NULL;

    return 0;
}

int lgh_checks(
    log_handler const* handler,
    __attribute__((unused))size_t handler_data_size,
    char const* caller_id
) {
    if (handler == NULL) {
        lgu_warn_msg_str("%s function was not given a handler pointer", caller_id);
        return 1;
    }
    else if (handler->handler_data_ == NULL) {
        lgu_warn_msg_str("%s function given handler that doesn't have data stored", caller_id);
        return 1;
    }
    /*
     * FIXME check below doesn't work as intended; sizeof(*(void*)) returns 1
    else if (sizeof(*(handler->handler_data_)) != handler_data_size) {
        lgu_warn_msg_str("%s function given handler with data of incorrect size", caller_id);
        return 1;
    }
    */

    return 0;
}

loghandler_t lgh_add_handler(log_handler const* handler) {

    if (_lgh_check_init()) {
        return kCloggerHandlerErr;
    }

    sem_wait(global_lock); // get the storage lock
    loghandler_t t_refRtn = 0;
    int ref_offset = 0;
    for (
        log_handler** handler_ptr = global_handler_storage;
        handler_ptr < global_last_handler;
        ++handler_ptr
    ) {
        // look for empty space for the handler
        if (*handler_ptr == NULL) {
            // allocate space for the new handler
            *handler_ptr = (log_handler*)malloc(sizeof(log_handler));
            if (*handler_ptr == NULL) {
                lgu_warn_msg("failed to allocate space for new handler");
                break;
            }
            // use memcpy to set due to const ptrs
            memcpy(*handler_ptr, handler, sizeof(log_handler));
            ++global_num_handlers;
            // mark the handler as valid in the global reference tracker
            t_refRtn = (1<<ref_offset);
            global_valid_handler_refs |= t_refRtn;
            break;
        }
        ++ref_offset;
    }
    sem_post(global_lock);

    if (t_refRtn == 0) return kCloggerHandlerErr;
    else return t_refRtn;
}

int lgh_get_num_handlers() {

    if (_lgh_check_init()) {
        return -1;
    }

    sem_wait(global_lock);
    int num_handlers = global_num_handlers;
    sem_post(global_lock);

    return num_handlers;
}

loghandler_t lgh_get_valid_handlers() {

    if (_lgh_check_init()) {
        return kCloggerHandlerErr;
    }

    loghandler_t t_refRtn = 0;

    sem_wait(global_lock);
    t_refRtn = global_valid_handler_refs;
    sem_post(global_lock);

    return t_refRtn;
}

int lgh_remove_all_handlers() {
    if (_lgh_check_init()) {
        return 1;
    }

    // if there aren't any handlers, nothing to do
    if (global_num_handlers == 0) return 0;

    int rtn_val = 0;
    sem_wait(global_lock); // get the storage lock

    for (
        log_handler** handler_ptr = global_handler_storage;
        handler_ptr < global_last_handler;
        ++handler_ptr
    ) {
        if (*handler_ptr != NULL) {
            if ((*handler_ptr)->isOpen(*handler_ptr)) {
                if ((*handler_ptr)->close(*handler_ptr)) {
                    // failed to close a handler
                    ++rtn_val;
                }
            }
            free(*handler_ptr);
            *handler_ptr = NULL;
            --global_num_handlers;
        }
    }
    global_valid_handler_refs = 0;
    sem_post(global_lock);

    return rtn_val;
}

int lgh_remove_handler(loghandler_t handler_ref) {
    // TODO in theory this function could support removing
    // multiple handlers at the same time, but for now we will
    // force it to only accept one
    if (_lgh_check_init()) {
        return 1;
    }
    else if (handler_ref & ~global_valid_handler_refs) {
        // the loghandler_t given isn't valid
        lgu_warn_msg("asked to remove a handler with an invalid reference");
    }

    int rtn_val = 0;
    int ref_offset = 0;
    sem_wait(global_lock); // get the storage lock
    for (
        log_handler** handler_ptr = global_handler_storage;
        handler_ptr < global_last_handler;
        ++handler_ptr
    ) {
        if ((*handler_ptr != NULL) && (handler_ref & (1<<ref_offset))) {
            if (handler_ref != (loghandler_t)(1<<ref_offset)) {
                // we were asked to remove more than one handler
                lgu_warn_msg("asked to remove more than one handler");
                rtn_val = 1;
                break;
            }

            // check if the handler is open
            if ((*handler_ptr)->isOpen(*handler_ptr)) {
                // close the handler
                (*handler_ptr)->close(*handler_ptr);
            }

            // free the memory for the handler
            free(*handler_ptr);
            *handler_ptr = NULL;
            --global_num_handlers;
            if (global_num_handlers < 0) {
                global_num_handlers = 0;
            }
            break;
        }
        ++ref_offset;
    }
    sem_post(global_lock);

    return rtn_val;
}

int lgh_write(loghandler_t handler_ref, t_loggermsg const* msg) {
    if (_lgh_check_init()) {
        return 1;
    }
    else if (handler_ref & ~global_valid_handler_refs) {
        // the loghandler_t given isn't valid
        lgu_warn_msg("asked to write to a handler with an invalid reference");
        return 1;
    }

    int rtn_val = 0;
    int ref_offset = 0;
    // TODO if each handler gets its own read/write lock, this won't be needed
    sem_wait(global_lock); // get the storage lock
    for (
        log_handler** handler_ptr = global_handler_storage;
        handler_ptr < global_last_handler;
        ++handler_ptr
    ) {
        if ((*handler_ptr != NULL) && (handler_ref & (1<<ref_offset))) {
            // check if the handler needs to be opened
            if (!(*handler_ptr)->isOpen(*handler_ptr)) {
                // try to open the handler
                if ((*handler_ptr)->open(*handler_ptr)) {
                    lgu_warn_msg("asked to write to a handler that isn't open, and it couldn't be opened");
                    ++rtn_val;
                    continue;
                }
            }
            // write the message to the handler
            if ((*handler_ptr)->write(*handler_ptr, msg)) {
                // failed to write to a handler; indicate an error, but keep going
                ++rtn_val;
            }
        }
        if (handler_ref <= (loghandler_t)(1<<ref_offset)) {
            // there aren't any higher bits set
            break;
        }
        ++ref_offset;
    }
    sem_post(global_lock);

    return rtn_val;
}

int lgh_write_to_all(t_loggermsg const* msg) {
    if (_lgh_check_init()) {
        return 1;
    }

    int num_handlers_written = 0;

    // TODO if each handler gets its own read/write lock, this won't be needed
    sem_wait(global_lock); // get the storage lock
    int index = 0;
    for (
        log_handler** handler_ptr = global_handler_storage;
        handler_ptr < global_last_handler;
        ++handler_ptr
    ) {
        if (*handler_ptr != NULL) {
            if ((*handler_ptr)->isOpen(*handler_ptr)) {
                if ((*handler_ptr)->write(*handler_ptr, msg)) {
                    // failed to write to a handler
                    // TODO could we identify the handler that failed?
                    lgu_warn_msg_int("failed to write to open handler at index %d", index);
                }
                else {
                    ++num_handlers_written;
                }
            }
        }
        ++index;
    }
    sem_post(global_lock);

    if (num_handlers_written) {
        return 0;
    }
    else {
        return 1;
    }
}

#ifdef __cplusplus
log_handler::log_handler(
    int (*const writep)(struct log_handler*, const t_loggermsg*),
    int (*const closep)(struct log_handler*),
    int (*const openp)(struct log_handler*),
    int (*const isOpenp)(const struct log_handler*),
    bool allow_empty_line,
    bool add_format,
    void *handler_data
) :
    write(writep), close(closep), open(openp), isOpen(isOpenp)
{
  this->allow_empty_line_ = allow_empty_line;
  this->add_format_ = add_format;
  this->handler_data_ = handler_data;
}

log_handler::log_handler() :
    write(nullptr), close(nullptr), open(nullptr), isOpen(nullptr)
{
  this->allow_empty_line_ = false;
  this->add_format_ = false;
  this->handler_data_ = nullptr;
}

#endif  // __cplusplus

__MALORGITH_NAMESPACE_CLOSE
