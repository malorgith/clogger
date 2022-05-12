
#include "logger_buffer.h"

#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdlib.h>

#ifndef LOGGER_SLEEP_SECS
#define LOGGER_SLEEP_SECS 1
#endif

#ifndef BUFFER_CLOSE_WARN
#define BUFFER_CLOSE_WARN 5 // stop adding messages to buffer when there's this number or fewer free spaces
#endif

#ifndef LOGGER_BUFFER_MAX_NUM_BUFFERS
#define LOGGER_BUFFER_MAX_NUM_BUFFERS 3
#endif

#if BUFFER_CLOSE_WARN > CLOGGER_BUFFER_SIZE
#error "BUFFER_CLOSE_WARN must be less than CLOGGER_BUFFER_SIZE"
#endif

#if BUFFER_CLOSE_WARN <= 0
#error "BUFFER_CLOSE_WARN must be > 0"
#endif

typedef struct {
    t_loggermsg*    messages[CLOGGER_BUFFER_SIZE];
    sem_t           rlock;
    sem_t           wlock;
    atomic_int      amsgs;
    atomic_int      arindx;
    atomic_int      awindx;
} logger_buffer;

// global variables
static logger_buffer** buffers = { NULL };
static sem_t* g_pStorageSem = { NULL };

// private function declarations
static int _lgb_check_values(int bufref);
static int _lgb_get_new_index(int current_index);

// private function definitions
int _lgb_check_values(int bufref) {

    char* err_msg = NULL;
    if (buffers == NULL) {
        err_msg = (char*) "there are no buffers defined.";
    }
    else if (g_pStorageSem == NULL) {
        err_msg = (char*) "the global buffer lock doesn't exist.";
    }
    else if (bufref >= 0) {
        if (bufref >= LOGGER_BUFFER_MAX_NUM_BUFFERS) {
            err_msg = "the buffer reference is too large";
        }
        else if (buffers[bufref] == NULL) {
            err_msg = (char*) "the buffer referenced doesn't exist.";
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
    else if (g_pStorageSem != NULL) {
        lgu_warn_msg("the lock to modify storage already exists.");
        return -1;
    }

    g_pStorageSem = (sem_t*) malloc(sizeof(sem_t));
    sem_init(g_pStorageSem, 0, 0);

    buffers = (logger_buffer**) malloc(sizeof(logger_buffer*) * LOGGER_BUFFER_MAX_NUM_BUFFERS);
    if (buffers == NULL) {
        lgu_warn_msg("failed to allocate space for the log buffers.");
        return -1;
    }

    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++)
        buffers[count] = NULL;

    sem_post(g_pStorageSem);
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
     * we don't need to get g_pStorageSem because it will be grabbed each time
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

    sem_destroy(g_pStorageSem);
    free(g_pStorageSem);

    free(buffers);
    buffers = NULL;

    return rtn_val;
}

int lgb_create_buffer() {

    if (buffers == NULL) {
        lgu_warn_msg("no space has been allocated for buffers.");
        return -1;
    }

    sem_wait(g_pStorageSem);

    int buf_count = -1;
    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++) {
        if (buffers[count] == NULL) {
            buf_count = count;
            break;
        }
    }
    if (buf_count == -1) {
        lgu_warn_msg("there's no space available for the new buffer.");
        sem_post(g_pStorageSem);
        return -1;
    }

    buffers[buf_count] = (logger_buffer*) malloc(sizeof(logger_buffer));
    if (buffers[buf_count] == NULL) {
        lgu_warn_msg("failed to allocate space for new buffer.");
        sem_post(g_pStorageSem);
        return -1;
    }

    buffers[buf_count]->arindx = 0;
    buffers[buf_count]->awindx = 0;

    for (int count = 0; count < CLOGGER_BUFFER_SIZE; count++)
        buffers[buf_count]->messages[count] = NULL;

    if (sem_init(&buffers[buf_count]->rlock, 0, 1)) {
        lgu_warn_msg_int("Failed to create the rlock; errno: %d.", errno);
        sem_post(g_pStorageSem);
        return -1;
    }

    if (sem_init(&buffers[buf_count]->wlock, 0, 1)) {
        lgu_warn_msg_int("Failed to create the wlock; errno: %d.", errno);
        sem_post(g_pStorageSem);
        return -1;
    }

    buffers[buf_count]->amsgs = 0;

    sem_post(g_pStorageSem);

    return buf_count;
}

int lgb_remove_buffer(int bufref) {

    if (_lgb_check_values(bufref))
        return 1;

    // get the global lock to modify storage
    sem_wait(g_pStorageSem);

    // get both locks on the buffer that will be removed
    sem_wait(&buffers[bufref]->rlock);
    sem_wait(&buffers[bufref]->wlock);

    // look for any messages on the buffer and free them
    int t_nMsgDropped = 0;
    for (int count = 0; count < CLOGGER_BUFFER_SIZE; count++) {
        if (buffers[bufref]->messages[count] != NULL) {
            free(buffers[bufref]->messages[count]);
            buffers[bufref]->messages[count] = NULL;
            t_nMsgDropped++;
        }
    }

    if (t_nMsgDropped) {
        lgu_warn_msg_int("'%d' messages were dropped while buffer was being destroyed", t_nMsgDropped);
    }

    buffers[bufref]->amsgs = 0;

    // destroy the buffer's locks
    sem_destroy(&buffers[bufref]->rlock);
    sem_destroy(&buffers[bufref]->wlock);

    // free the memory used by the buffer
    free(buffers[bufref]);
    buffers[bufref] = NULL;

    // let go of the storage modification lock
    sem_post(g_pStorageSem);

    return 0;
}

int lgb_add_message(int bufref, t_loggermsg* msg) {

    if (_lgb_check_values(bufref))
        return 1;

    sem_wait(&buffers[bufref]->wlock); // get the lock

    // make sure there aren't too many messages on the buffer
    if ((CLOGGER_BUFFER_SIZE - buffers[bufref]->amsgs) <= BUFFER_CLOSE_WARN) {
        lgu_warn_msg("there are too many unread messages.");
        sem_post(&buffers[bufref]->wlock);
        return 1;
    }

    buffers[bufref]->messages[buffers[bufref]->awindx] = msg; // add the message
    buffers[bufref]->awindx = _lgb_get_new_index(buffers[bufref]->awindx); // increment the write index
    buffers[bufref]->amsgs++; // increment the number of unread messages
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
    rtn_val = buffers[bufref]->messages[read_index]; // get the pointer to the message
    buffers[bufref]->messages[read_index] = NULL; // remove the message from the buffer
    buffers[bufref]->arindx = _lgb_get_new_index(buffers[bufref]->arindx); // increment the read index
    buffers[bufref]->amsgs--; // decrease count of unread messages
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

    static const int max_milliseconds_wait = 3000;
    static const int min_milliseconds_wait = 1;

    if (_lgb_check_values(bufref))
        return -1;

    // before doing anything else, check if there's already a message
    if (buffers[bufref]->amsgs > 0) {
        return 0;
    }

    // get the current time
    struct timespec break_time;
    if (clock_gettime(CLOCK_REALTIME, &break_time) == 1) {
        lgu_warn_msg("something went wrong getting the time");
        return -1;
    }

    // determine the time to break from the loop
    lgb_add_to_time(&break_time, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);

    while(buffers[bufref]->amsgs == 0) {
        struct timespec loop_time;
        if (clock_gettime(CLOCK_REALTIME, &loop_time) == 1) {
            lgu_warn_msg("something went wrong getting the time");
            return -1;
        }
        // check if we've looped for enough time
        if (
            (loop_time.tv_sec >= break_time.tv_sec) &&
            (loop_time.tv_nsec >= break_time.tv_nsec)
        ) {
            return 1; // return 1 to indicate max time elapsed
        }
    }

    if (buffers[bufref]->amsgs > 0)
        return 0; // there's at least one message

    // we should have returned from the loop above after timing out, so indicate error
    return -1;
}

int lgb_add_to_time(struct timespec *p_pTspec, int ms_to_add, int min_ms, int max_ms) {

    if (ms_to_add > max_ms) {
        lgu_warn_msg_int("Maximum time to wait is %d milliseconds.", max_ms);
        ms_to_add = max_ms;
    }
    else if (ms_to_add < min_ms) {
        lgu_warn_msg_int("Minimum time to wait is %d milliseconds.", min_ms);
        ms_to_add = min_ms;
    }

    // convert the milliseconds to nanoseconds
    long sleep_time_nsecs = ms_to_add * (long) 1000000;
    int sleep_time_secs = 0;
    while (sleep_time_nsecs >= (long) 1000000000) {
        sleep_time_nsecs -= (long) 1000000000;
        sleep_time_secs++;
    }

    // add the time to the struct
    p_pTspec->tv_sec += sleep_time_secs;
    p_pTspec->tv_nsec += sleep_time_nsecs;
    if (p_pTspec->tv_nsec >= (long) 1000000000) {
        p_pTspec->tv_nsec -= (long) 1000000000;
        p_pTspec->tv_sec += 1;
    }

    return 0;
}

#ifndef NDEBUG
int logger_get_buffer_size() {
    return CLOGGER_BUFFER_SIZE;
}

int logger_get_buffer_close_warn() {
    return BUFFER_CLOSE_WARN;
}
#endif

