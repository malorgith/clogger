
#include "logger_buffer.h"

#include <errno.h>
#include <semaphore.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <cstring>
#else
#include <string.h>
#endif  // __cplusplus

#include "logger_atomic.h"
#include "logger_defines.h"

// stop adding messages to buffer when there's this number or fewer free spaces
#ifndef BUFFER_CLOSE_WARN
#define BUFFER_CLOSE_WARN 5
#endif // BUFFER_CLOSE_WARN

#ifndef LOGGER_BUFFER_MAX_NUM_BUFFERS
#define LOGGER_BUFFER_MAX_NUM_BUFFERS 3
#endif // LOGGER_BUFFER_MAX_NUM_BUFFERS

#if BUFFER_CLOSE_WARN > CLOGGER_BUFFER_SIZE
#error "BUFFER_CLOSE_WARN must be less than CLOGGER_BUFFER_SIZE"
#endif

#if BUFFER_CLOSE_WARN <= 0
#error "BUFFER_CLOSE_WARN must be > 0"
#endif

__MALORGITH_NAMESPACE_OPEN
#ifdef __cplusplus
namespace {
#endif // __cplusplus

typedef struct {
    t_loggermsg messages[CLOGGER_BUFFER_SIZE]; /*!< message storage */
    sem_t rlock;  /*!< read lock */
    sem_t wlock;  /*!< write lock */
    MALORGITH_CLOGGER_ATOMIC_INT amsgs;  /*!< number of unread messages */
    MALORGITH_CLOGGER_ATOMIC_INT arindx; /*!< read index */
    MALORGITH_CLOGGER_ATOMIC_INT awindx; /*!< write index */
} logger_buffer;

// global variables
static logger_buffer** buffers = { NULL };
static sem_t* storage_sem = { NULL };

// private function declarations
static int _lgb_check_values(int bufref);
static int _lgb_get_new_index(int current_index);

// private function definitions
int _lgb_check_values(int bufref) {

    char const* err_msg = NULL;
    if (buffers == NULL) {
        err_msg = "there are no buffers defined.";
    }
    else if (storage_sem == NULL) {
        err_msg = "the global buffer lock doesn't exist.";
    }
    else if (bufref >= 0) {
        if (bufref >= LOGGER_BUFFER_MAX_NUM_BUFFERS) {
            err_msg = "the buffer reference is too large";
        }
        else if (buffers[bufref] == NULL) {
            err_msg = "the buffer referenced doesn't exist.";
        }
    }

    if (err_msg != NULL) {
        lgu_warn_msg(err_msg);
        return 1;
    }

    return 0;
}

static int _lgb_get_new_index(int current_index) {
    int new_index = current_index + 1;
    if (new_index >= CLOGGER_BUFFER_SIZE) {
        return 0; // roll the value over to 0 if we reached the max size
    }
    return new_index;
}

#ifdef __cplusplus
}  // namespace
#endif  // __cplusplus

// public functions
int lgb_init() {

    if (BUFFER_CLOSE_WARN > CLOGGER_BUFFER_SIZE) {
        lgu_warn_msg("Buffer warning size can't be greater than buffer size");
        return -1;
    }
    else if (BUFFER_CLOSE_WARN <= 0) {
        lgu_warn_msg("Buffer warning size must be greater than zero");
        return -1;
    }
    else if ((CLOGGER_BUFFER_SIZE - BUFFER_CLOSE_WARN) <= 0) {
        lgu_warn_msg("Buffer warning value must be below buffer size");
        return -1;
    }

    if (buffers != NULL) {
        lgu_warn_msg("buffers have already been allocated.");
        return -1;
    }
    else if (storage_sem != NULL) {
        lgu_warn_msg("the lock to modify storage already exists.");
        return -1;
    }

    storage_sem = (sem_t*) malloc(sizeof(sem_t));
    sem_init(storage_sem, 0, 0);

    buffers = (logger_buffer**) malloc(sizeof(logger_buffer*) * LOGGER_BUFFER_MAX_NUM_BUFFERS);
    if (buffers == NULL) {
        lgu_warn_msg("failed to allocate space for the log buffers.");
        return -1;
    }

    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++)
        buffers[count] = NULL;

    sem_post(storage_sem);
    int buffer_create_rtn = lgb_create_buffer();
    if (buffer_create_rtn < 0) {
        lgu_warn_msg("couldn't create the default buffer.");
        return -1;
    }

    return buffer_create_rtn;
}

int lgb_free() {

    if (buffers == NULL) {
        lgu_warn_msg("No buffers to free.");
        return 1;
    }

    /*
     * we don't need to get storage_sem because it will be grabbed each time
     * lgb_remove_buffer() is called
     */
    int rtn_val = 0;
    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++) {
        if (buffers[count] != NULL) {
            if (lgb_remove_buffer(count)) {
                lgu_warn_msg_int("Failed to free buffer at reference: %d", count);
                rtn_val = 1;
            }
        }
    }

    sem_destroy(storage_sem);
    free(storage_sem);
    storage_sem = NULL;

    free(buffers);
    buffers = NULL;

    return rtn_val;
}

int lgb_create_buffer() {

    if (buffers == NULL) {
        lgu_warn_msg("no space has been allocated for buffers.");
        return -1;
    }

    sem_wait(storage_sem);

    int buf_count = -1;
    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++) {
        if (buffers[count] == NULL) {
            buf_count = count;
            break;
        }
    }
    if (buf_count == -1) {
        lgu_warn_msg("there's no space available for the new buffer.");
        sem_post(storage_sem);
        return -1;
    }

    buffers[buf_count] = (logger_buffer*) malloc(sizeof(logger_buffer));
    if (buffers[buf_count] == NULL) {
        lgu_warn_msg("failed to allocate space for new buffer.");
        sem_post(storage_sem);
        return -1;
    }

    buffers[buf_count]->arindx = 0;
    buffers[buf_count]->awindx = 0;

    if (sem_init(&buffers[buf_count]->rlock, 0, 1)) {
        lgu_warn_msg_int("Failed to create the rlock; errno: %d.", errno);
        sem_post(storage_sem);
        return -1;
    }

    if (sem_init(&buffers[buf_count]->wlock, 0, 1)) {
        lgu_warn_msg_int("Failed to create the wlock; errno: %d.", errno);
        sem_post(storage_sem);
        return -1;
    }

    buffers[buf_count]->amsgs = 0;

    sem_post(storage_sem);

    return buf_count;
}

int lgb_remove_buffer(int bufref) {

    if (_lgb_check_values(bufref))
        return 1;

    // get the global lock to modify storage
    sem_wait(storage_sem);

    // get both locks on the buffer that will be removed
    sem_wait(&buffers[bufref]->rlock);
    sem_wait(&buffers[bufref]->wlock);

    // look for any messages on the buffer and free them
    if (buffers[bufref]->amsgs) {
        lgu_warn_msg_int("'%d' messages were dropped while buffer was being destroyed", buffers[bufref]->amsgs);
        buffers[bufref]->amsgs = 0;
    }

    // destroy the buffer's locks
    sem_destroy(&buffers[bufref]->rlock);
    sem_destroy(&buffers[bufref]->wlock);

    // free the memory used by the buffer
    free(buffers[bufref]);
    buffers[bufref] = NULL;

    // let go of the storage modification lock
    sem_post(storage_sem);

    return 0;
}

int lgb_add_message(int bufref, t_loggermsg* msg) {

    if (_lgb_check_values(bufref))
        return 1;

    sem_wait(&buffers[bufref]->wlock); // get the lock

    // make sure there aren't too many messages on the buffer
    if ((CLOGGER_BUFFER_SIZE - buffers[bufref]->amsgs) <= BUFFER_CLOSE_WARN) {
        lgu_warn_msg("there are too many unread messages");
        sem_post(&buffers[bufref]->wlock);
        return 1;
    }

    buffers[bufref]->messages[buffers[bufref]->awindx] = *msg; // add the message
    buffers[bufref]->awindx = _lgb_get_new_index(buffers[bufref]->awindx); // increment the write index
    ++buffers[bufref]->amsgs; // increment the number of unread messages
    sem_post(&buffers[bufref]->wlock); // release the lock

    return 0;
}

/*
 * Removes a message from the buffer and returns a pointer to it.
 */
t_loggermsg* lgb_read_message(int bufref) {

    if (_lgb_check_values(bufref))
        return NULL;

    t_loggermsg* rtn_val = NULL;

    sem_wait(&buffers[bufref]->rlock); // get the lock

    if (buffers[bufref]->amsgs == 0) {
        lgu_warn_msg("there are no unread messages.");
        sem_post(&buffers[bufref]->rlock);
        return NULL;
    }

    int read_index = buffers[bufref]->arindx; // get the current read index
    rtn_val = &buffers[bufref]->messages[read_index]; // get the pointer to the message
    buffers[bufref]->arindx = _lgb_get_new_index(buffers[bufref]->arindx); // increment the read index
    --buffers[bufref]->amsgs; // decrease count of unread messages
    sem_post(&buffers[bufref]->rlock); // release the lock

    return rtn_val;
}

/*
 * Waits for milliseconds_to_wait milliseconds for a message to appear on the buffer.
 *
 * Returns:
 * 0  when there's a message to be read
 * >0 when the wait timed out
 * <0 when there was an error
 */
int lgb_wait_for_messages(int bufref, int milliseconds_to_wait) {

    // FIXME this value is specified twice; here and in logger.c called g_nMaxSleepTime
    static const int max_milliseconds_wait = 5000;
    static const int min_milliseconds_wait = 1;

    if (_lgb_check_values(bufref))
        return -1;

    sem_wait(&buffers[bufref]->rlock); // get the lock to make sure the buffer isn't removed

    // before doing anything else, check if there's already a message
    if (buffers[bufref]->amsgs > 0) {
        sem_post(&buffers[bufref]->rlock);
        return 0;
    }

    // get the current time
    struct timespec break_time = {0, 0};

    #ifdef CLOGGER_NO_SLEEP
    if (clock_gettime(CLOCK_REALTIME, &break_time) == 1) {
        lgu_warn_msg("something went wrong getting the time");
        sem_post(&buffers[bufref]->rlock);
        return -1;
    }

    // determine the time to break from the loop
    lgu_add_to_time(&break_time, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);

    while(buffers[bufref]->amsgs == 0) {
        struct timespec loop_time;
        if (clock_gettime(CLOCK_REALTIME, &loop_time) == 1) {
            lgu_warn_msg("something went wrong getting the time");
            sem_post(&buffers[bufref]->rlock);
            return -1;
        }
        // check if we've looped for enough time
        if (
            (loop_time.tv_sec >= break_time.tv_sec) &&
            (loop_time.tv_nsec >= break_time.tv_nsec)
        ) {
            sem_post(&buffers[bufref]->rlock);
            return 1; // return 1 to indicate max time elapsed
        }
    }
    #else
    lgu_add_to_time(&break_time, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);
    // sleep for the time specified
    if (nanosleep(&break_time, NULL)) {
        // interrupted by signal or enountered an error
        sem_post(&buffers[bufref]->rlock);
        return -1;
    }

    if (buffers[bufref]->amsgs <= 0) {
        sem_post(&buffers[bufref]->rlock);
        return 1; // max time elapsed
    }
    #endif  // CLOGGER_NO_SLEEP

    if (buffers[bufref]->amsgs > 0) {
        sem_post(&buffers[bufref]->rlock);
        return 0; // there's at least one message
    }

    // we should have returned from the loop above after timing out, so indicate error
    sem_post(&buffers[bufref]->rlock);
    return -1;
}

#ifndef NDEBUG
int logger_get_buffer_size() {
    return CLOGGER_BUFFER_SIZE;
}

int logger_get_buffer_close_warn() {
    return BUFFER_CLOSE_WARN;
}
#endif // NDEBUG

__MALORGITH_NAMESPACE_CLOSE
