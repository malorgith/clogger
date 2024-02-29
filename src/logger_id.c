
#include "logger_id.h"

#include <semaphore.h>
#include <stdlib.h>
#include <string.h> // strlen()
#include <time.h>

//#include "clogger.h"
#include "logger_atomic.h"
#include "logger_defines.h"
#include "logger_util.h"
#include __MALORGITH_CLOGGER_INCLUDE

__MALORGITH_NAMESPACE_OPEN
#ifdef __cplusplus
namespace {
#endif // __cplusplus

// global variables
static int volatile log_id_running = { 0 };
static int global_longest_id_len = { 0 };

typedef struct {
    /*! number of messages waiting to be processed */
    MALORGITH_CLOGGER_ATOMIC_INT num_waiting_msgs;
    /*! indicates if the ID is accepting messages */
    bool accept_msgs;
    /*! the ID string */
    char str_id[CLOGGER_ID_MAX_LEN];
    /*! lock for modifying the ID */
    sem_t lock;
    /*! the handlers the ID writes to */
    loghandler_t handlers;
} loggerid_struct;

/*
 * TODO
 * Move num_waiting_msgs out of the struct
 * and create a global variable array of atomic_ints
 * that serves the same purpose. This will reduce
 * the number of checks and locks that will be required
 * before increasing or decreasing the value.
 */

/* TODO
 * MAYBE
 * We could move the calls for the handlers to write from
 * logger.c into here.
 * Potential downside of holding on to the logid_t locks longer,
 * but maybe we reduce the possibility of having a handler removed
 * while we're attempting to log to it?
 * With the num_waiting_msgs moved to global array it wouldn't be
 * as bad for us to hold on to the lock for a bit, maybe.
 */

loggerid_struct   **log_id_storage = { NULL };
static sem_t*       storage_lock = { NULL };

// private function declarations
static int _lgi_checks(logid_t, char const*);
static loggerid_struct* _lgi_create_id(char const* str_id, loghandler_t handlers);
static int _lgi_update_existing_vals(size_t new_len);
static int _lgi_remove_all_messages(logid_t);

// private function definitions
/*
 * The function below checks the sanity of the global variables,
 * makes sure we're running, and _GRABS THE LOCK_ on the id
 * specified at the reference.
 */
static int _lgi_checks(logid_t id_index, char const* str_err_msg) {
    if (!log_id_running) {
        lgu_warn_msg_str("%s: logger ids aren't running", str_err_msg);
        return 1;
    }
    else if (log_id_storage == NULL) {
        lgu_warn_msg_str("%s: space for logger ids hasn't been allocated", str_err_msg);
        return 1;
    }
    else if (log_id_storage[id_index] == NULL) {
        lgu_warn_msg_str("%s: id reference given points to null", str_err_msg);
        return 1;
    }

    // get the lock on the id
    // TODO timedwait ?
    sem_wait(&log_id_storage[id_index]->lock);

    return 0;
}

loggerid_struct* _lgi_create_id(char const* str_id, loghandler_t handlers) {
    // allocate space for the ID
    loggerid_struct* logid = (loggerid_struct*) malloc(sizeof(loggerid_struct));
    if (logid == NULL) {
        lgu_warn_msg("failed to allocate space for a new id");
        return NULL;
    }

    // store the string given in the struct
    static char const* str_err_msg = "failed to store the id given";
    if (lgu_wsnprintf(logid->str_id, CLOGGER_ID_MAX_LEN, str_err_msg, "%s", str_id)) {
        // failed to store the string given; free resources and fail
        free(logid);
        return NULL;
    }

    // init other values in struct
    logid->num_waiting_msgs = 0;
    logid->accept_msgs = true;
    logid->handlers = handlers;
    sem_init(&logid->lock, 0, 1);

    // check if we need to add space padding to existing values
    /*** the new id's lock must be initialized before we do this ***/
    int id_len = strlen(logid->str_id);
    if (id_len > global_longest_id_len) {
        // the new ID is longer than any previous one; space pad them to match
        global_longest_id_len = id_len;
        _lgi_update_existing_vals(global_longest_id_len);
    }
    else if (id_len < global_longest_id_len) {
        int index;
        for (index = id_len; index < global_longest_id_len; index++) {
            logid->str_id[index] = ' ';
        }
        logid->str_id[index] = '\0';
    }

    return logid;
}

int _lgi_remove_all_messages(logid_t id_index) {

    // do sanity checks and get object's lock
    if (_lgi_checks(id_index, "can't clear waiting messages"))
        return 1;

    log_id_storage[id_index]->num_waiting_msgs = 0;
    sem_post(&log_id_storage[id_index]->lock);

    return 0;
}

int _lgi_update_existing_vals(size_t new_len) {
    /* this function expects the storage lock to already be required */
    size_t size_of_char = sizeof(char);
    for (int count = 0; count < CLOGGER_MAX_NUM_IDS; count++) {
        if (log_id_storage[count] != NULL) {
            // do sanity check and get id's lock
            if (_lgi_checks(count, "can't update existing id"))
                return 1;
            size_t len = strlen(log_id_storage[count]->str_id);
            int new_final_index = (int) ((len / size_of_char) + ((new_len - len) / size_of_char));
            int index;
            for (index = len; index < new_final_index; index++) {
                log_id_storage[count]->str_id[index] = ' ';
            }
            log_id_storage[count]->str_id[index] = '\0';

            // free the object's lock
            sem_post(&log_id_storage[count]->lock);
        }
    }
    return 0;
}

#ifdef __cplusplus
}  // namespace
#endif  // __cplusplus

// public functions
int lgi_init() {

    if (log_id_running) {
        lgu_warn_msg("log ids already initialized");
        return 1;
    }
    else if (log_id_storage != NULL) {
        lgu_warn_msg("log ids aren't running, but there's data stored");
        return 1;
    }

    // allocate space to store the IDs
    log_id_storage = (loggerid_struct**) malloc(sizeof(loggerid_struct*) * CLOGGER_MAX_NUM_IDS);
    if (log_id_storage == NULL) {
        lgu_warn_msg("failed to allocate space to store logger ids");
        return 1;
    }

    // init each id location to NULL
    for (int id_index = 0; id_index < CLOGGER_MAX_NUM_IDS; id_index++) {
        log_id_storage[id_index] = NULL;
    }

    storage_lock = (sem_t*) malloc(sizeof(sem_t));

    if (storage_lock == NULL) {
        lgu_warn_msg("logger id failed to allocate space for storage lock");
        free(log_id_storage);
        log_id_storage = NULL;
        return 1;
    }
    sem_init(storage_lock, 0, 1);

    log_id_running = 1;

    return 0;
}

int lgi_free() {

    if (!log_id_running) {
        lgu_warn_msg("asked to free logger ids, but they weren't initialized");
        // TODO is this an error?
        return 0;
    }
    else if (log_id_storage == NULL) {
        lgu_warn_msg("asked to free logger ids, and we thought we're running, but ids are null");
        // this shouldn't happen, so it's an error
        return 1;
    }

    // TODO it would be good to set log_id_running = 0 here, but lgi_remove_id() will
    // get upset (through _lgi_checks()) if we aren't running

    /*
     * we can't get the storage lock yet because we get that in lgi_remove_id(),
     * but by setting log_id_running to false we should ensure no new messages or ids
     * come in
     */

    // remove all active ids
    for (logid_t id_index = 0; id_index < CLOGGER_MAX_NUM_IDS; id_index++) {
        if (log_id_storage[id_index]) {
        /*
         * This function is called after the logger has already started its shutdown process.
         * Clear the value of num_waiting_msgs for each ID since no more messages will be
         * written.
         */
            _lgi_remove_all_messages(id_index);
            lgi_remove_id(id_index);
        }
    }

    log_id_running = 0;

    // get the storage lock
    // TODO timedwait ?
    sem_wait(storage_lock);

    free(log_id_storage);
    log_id_storage = NULL;

    global_longest_id_len = 0;

    sem_destroy(storage_lock);
    free(storage_lock);
    storage_lock = NULL;
    return 0;
}

int lgi_add_handler(logid_t id_index, loghandler_t p_refHandler) {

    // do sanity checks and get object's lock
    if (_lgi_checks(id_index, "can't add handler"))
        return 1;
    // TODO should we check if it's still accepting messages? adding a handler won't keep it from freeing

    log_id_storage[id_index]->handlers |= p_refHandler;

    // TODO any sort of checks we can do? we don't know the valid handlers here

    sem_post(&log_id_storage[id_index]->lock);

    return 0;
}

logid_t lgi_add_id(char const* str_id) {
    return lgi_add_id_w_handler(str_id, 0);
}

logid_t lgi_add_id_w_handler(char const* str_id, loghandler_t p_refHandlers) {

    if (!log_id_running) {
        lgu_warn_msg("asked to add an id, but we're not running");
        return CLOGGER_MAX_NUM_IDS;
    }
    else if (log_id_storage == NULL) {
        lgu_warn_msg("asked to add an id, and think we're running, but no space allocated for ids");
        return CLOGGER_MAX_NUM_IDS;
    }

    // TODO timedwait?
    sem_wait(storage_lock);

    // find an empty slot for the id
    logid_t id_index;
    for (id_index = 0; id_index < CLOGGER_MAX_NUM_IDS; id_index++) {
        if (log_id_storage[id_index] == NULL) {
            if ((log_id_storage[id_index] = _lgi_create_id(str_id, p_refHandlers)) == NULL) {
                id_index = CLOGGER_MAX_NUM_IDS;
            }
            break;
        }
    }

    sem_post(storage_lock);
    return id_index;
}

int lgi_add_message(logid_t id_index) {

    // do sanity checks and get object's lock
    if (_lgi_checks(id_index, "can't increase message count"))
        return 1;
    else if (!log_id_storage[id_index]->accept_msgs) {
        lgu_warn_msg("asked to add message to ID that isn't accepting them");
        sem_post(&log_id_storage[id_index]->lock);
        return 1;
    }

    ++log_id_storage[id_index]->num_waiting_msgs;
    sem_post(&log_id_storage[id_index]->lock);

    return 0;
}

/*
 * This function retrieves the string value of the specified ID
 * and stores it in 'dest'.
 *
 * On success, returns the ID's handlers to tell the caller which
 * handlers should be written to. Returns 0, which isn't a valid
 * loghandler_t, on failure.
 */
loghandler_t lgi_get_id(logid_t id_index, char* dest) {

    static char const* str_err_msg = "failed to copy id to destination";
    loghandler_t t_refRtn = kCloggerHandlerErr;

    if (dest == NULL) {
        lgu_warn_msg("can't copy id to a null pointer");
        return 0;
    }
    else if (_lgi_checks(id_index, "can't get id"))
        return 0;

    if (!lgu_wsnprintf(dest, CLOGGER_ID_MAX_LEN, str_err_msg, "%s", log_id_storage[id_index]->str_id)) {
        // no error; update the return value while we have the lock
        t_refRtn = log_id_storage[id_index]->handlers;
    }

    sem_post(&log_id_storage[id_index]->lock);

    return t_refRtn;

}

int lgi_remove_handler(logid_t id_index, loghandler_t p_refHandler) {

    loghandler_t new_handlers = (log_id_storage[id_index]->handlers & ~p_refHandler);
    int rtn_val = 0;

    // do sanity check and get id's lock
    if (_lgi_checks(id_index, "can't remove id"))
        return 1;

    if (!log_id_storage[id_index]->accept_msgs) {
        lgu_warn_msg("Won't add handler to ID; ID isn't accepting messages.");
        rtn_val = 1;
    }
    else if ((new_handlers = log_id_storage[id_index]->handlers & ~p_refHandler) == 0) {
        // TODO should this error? no handlers is a valid state when the ID is created
        lgu_warn_msg("Can't remove handler from ID; ID would have no handlers left");
        rtn_val = 1;
    }
    else {
        log_id_storage[id_index]->handlers = new_handlers;
    }

    sem_post(&log_id_storage[id_index]->lock);

    return rtn_val;
}

int lgi_remove_id(logid_t id_index) {

    // do sanity check and get id's lock
    if (_lgi_checks(id_index, "can't remove id"))
        return 1;

    // tell the id to stop accepting new messages
    log_id_storage[id_index]->accept_msgs = false;

    // release the lock on the id that was acquired in _lgi_checks()
    sem_post(&log_id_storage[id_index]->lock);

    if (lgu_timedwait(storage_lock, 500)) {
        // failed to get the lock
        lgu_warn_msg("failed to get id storage lock to remove an id");
        return 1;
    }

    if (log_id_storage[id_index]->num_waiting_msgs > 0) {
        // there are messages waiting; sleep to give them time to process
        // get the current time
        int max_loops = 20;
        int loop_count = 0;
        while (log_id_storage[id_index]->num_waiting_msgs > 0) {
            struct timespec break_time = {0, 0};
            lgu_add_to_time(&break_time, 25, 1, 1000);  // FIXME(malorgith): use constants
            // sleep for the time specified
            if (nanosleep(&break_time, NULL)) {
                // interrupted by signal or enountered an error
                sem_post(storage_lock);
                return 1;
            }
            ++loop_count;
            if (loop_count >= max_loops) {
                break;
            }
        }
        if (log_id_storage[id_index]->num_waiting_msgs > 0) {
            // there are still messages waiting; proceed anyway
            lgu_warn_msg("messages still waiting for id that will be removed");
        }
    }

    // TODO timedwait ?
    sem_wait(&log_id_storage[id_index]->lock);
    sem_destroy(&log_id_storage[id_index]->lock);

    free(log_id_storage[id_index]);
    log_id_storage[id_index] = NULL;

    sem_post(storage_lock);

    return 0;
}

int lgi_remove_message(logid_t id_index) {

    // do sanity checks and get object's lock
    if (_lgi_checks(id_index, "can't remove a message"))
        return 1;
    else if (log_id_storage[id_index]->num_waiting_msgs < 1) {
        lgu_warn_msg("asked to decrease number of messages on ID that has less than one message");
        log_id_storage[id_index]->num_waiting_msgs = 0;
        sem_post(&log_id_storage[id_index]->lock);
        return 1;
    }

    --log_id_storage[id_index]->num_waiting_msgs;
    sem_post(&log_id_storage[id_index]->lock);

    return 0;
}

__MALORGITH_NAMESPACE_CLOSE
