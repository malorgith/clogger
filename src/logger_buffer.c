
#include "logger_buffer.h"

#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdlib.h>

#define BUFFER_SIZE 50
#define LOGGER_SLEEP_SECS 1
#define BUFFER_CLOSE_WARN 5 // stop adding messages to buffer when there's this number or fewer free spaces
#define LOGGER_BUFFER_MAX_NUM_BUFFERS 3

typedef struct {
    t_loggermsg*    messages[BUFFER_SIZE];
    sem_t           rlock;
    sem_t           wlock;
    atomic_int      amsgs;
    atomic_int      arindx;
    atomic_int      awindx;
} logger_buffer;

static logger_buffer** buffers = { NULL };
static sem_t* mod_storage = { NULL };

// PRIVATE FUNCTION DECLARATIONS
static int _lgbf_check_values(int bufref, char* err_msg_hdr);
static int _lgbf_get_new_index(int current_index);
// END PRIVATE FUNCTION DECLARATIONS

int _lgbf_check_values(int bufref, char* err_msg_hdr) {

    char* err_msg = NULL;
    if (buffers == NULL) {
        err_msg = (char*) "there are no buffers defined.";
    }
    else if (mod_storage == NULL) {
        err_msg = (char*) "the global buffer lock doesn't exist.";
    }
    else if (bufref >= 0) {
        if (buffers[bufref] == NULL) {
            err_msg = (char*) "the buffer referenced doesn't exist.";
        }
//      else if (buffers[bufref]->lock == NULL) {
//          err_msg = (char*) "the buffer's lock doesn't exist.";
//      }
    }

    if (err_msg != NULL) {
        if (err_msg_hdr != NULL) {
            fprintf(stderr, "%s %s", err_msg_hdr, err_msg);
        }
        else {
            fprintf(stderr, "logger buffer: %s\n", err_msg);
        }
        return 1;
    }

    return 0;
}

static int _lgbf_get_new_index(int current_index) {
    int new_index = current_index + 1;
    if (new_index >= BUFFER_SIZE) {
        return 0; // roll the value over to 0 if we reached the max size
    }
    return new_index;
}

int logger_buffer_init() {

    char* err_msg_hdr = (char*) "Failed to initialize buffers:";
    if (buffers != NULL) {
        fprintf(stderr, "%s buffers have already been allocated.\n", err_msg_hdr);
        return -1;
    }
    else if (mod_storage != NULL) {
        fprintf(stderr, "%s the lock to modify storage already exists.\n", err_msg_hdr);
        return -1;
    }

    mod_storage = (sem_t*) malloc(sizeof(sem_t));
    sem_init(mod_storage, 0, 0);

    buffers = (logger_buffer**) malloc(sizeof(logger_buffer*) * LOGGER_BUFFER_MAX_NUM_BUFFERS);
    if (buffers == NULL) {
        fprintf(stderr, "%s failed to allocate space for the log buffers.\n", err_msg_hdr);
        return -1;
    }

    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++)
        buffers[count] = NULL;

    sem_post(mod_storage);
    int buffer_create_rtn = logger_buffer_create_buffer();
    if (buffer_create_rtn < 0) {
        fprintf(stderr, "%s couldn't create the default buffer.\n", err_msg_hdr);
        return -1;
    }

    return buffer_create_rtn;
}

int logger_buffer_free() {

    if (buffers == NULL) {
        fprintf(stderr, "No buffers to free.\n");
        return 1;
    }

//  sem_wait(mod_storage); // TODO not sure why this was commented out

    int rtn_val = 0;
    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++) {
        if (buffers[count] != NULL) {
            if (logger_buffer_free_buffer(count)) {
                fprintf(stderr, "Failed to free buffer at reference: %d\n", count);
                rtn_val = 1;
            }
        }
    }

    sem_destroy(mod_storage);
    free(mod_storage);

    free(buffers);
    buffers = NULL;

    return rtn_val;
}

int logger_buffer_create_buffer() {

    char* err_msg_hdr = (char*) "Failed to create buffer:";
    if (buffers == NULL) {
        fprintf(stderr, "%s no space has been allocated for buffers.\n", err_msg_hdr);
        return -1;
    }

    sem_wait(mod_storage);

    int buf_count = -1;
    for (int count = 0; count < LOGGER_BUFFER_MAX_NUM_BUFFERS; count++) {
        if (buffers[count] == NULL) {
            buf_count = count;
            break;
        }
    }
    if (buf_count == -1) {
        fprintf(stderr, "%s there's no space available for the new buffer.\n", err_msg_hdr);
        sem_post(mod_storage);
        return -1;
    }

    buffers[buf_count] = (logger_buffer*) malloc(sizeof(logger_buffer));
    if (buffers[buf_count] == NULL) {
        fprintf(stderr, "%s failed to allocate space for new buffer.\n", err_msg_hdr);
        sem_post(mod_storage);
        return -1;
    }

    buffers[buf_count]->arindx = 0;
    buffers[buf_count]->awindx = 0;

    for (int count = 0; count < BUFFER_SIZE; count++)
        buffers[buf_count]->messages[count] = NULL;

//  buffers[buf_count]->lock = (sem_t*) malloc(sizeof(sem_t));
//  if (buffers[buf_count]->lock == NULL) {
//      fprintf(stderr, "%s failed to allocate space for the buffer's lock.\n", err_msg_hdr);
//      sem_post(mod_storage);
//      return -1;
//  }
//  if (sem_init(buffers[buf_count]->lock, 0, 1)) {
//      fprintf(stderr, "%s failed to create the lock; errno: %d.\n", err_msg_hdr, errno);
//      sem_post(mod_storage);
//      return -1;
//  }

    if (sem_init(&buffers[buf_count]->rlock, 0, 1)) {
        fprintf(stderr, "%s failed to create the rlock; errno: %d.\n", err_msg_hdr, errno);
        sem_post(mod_storage);
        return -1;
    }
    if (sem_init(&buffers[buf_count]->wlock, 0, 1)) {
        fprintf(stderr, "%s failed to create the wlock; errno: %d.\n", err_msg_hdr, errno);
        sem_post(mod_storage);
        return -1;
    }

    buffers[buf_count]->amsgs = 0;

    sem_post(mod_storage);

    return buf_count;
}

int logger_buffer_free_buffer(int bufref) {

    char* err_msg_hdr = (char*) "Can't free buffer:";
    if (_lgbf_check_values(bufref, err_msg_hdr))
        return 1;

    sem_wait(mod_storage);

    sem_wait(&buffers[bufref]->rlock);
    sem_wait(&buffers[bufref]->wlock);

    for (int count = 0; count < BUFFER_SIZE; count++) {
        if (buffers[bufref]->messages[count] != NULL) {
            // FIXME are we just going to discard these messages?
            free(buffers[bufref]->messages[count]);
            buffers[bufref]->messages[count] = NULL;
        }
    }

    buffers[bufref]->amsgs = 0;

    sem_destroy(&buffers[bufref]->rlock);
    sem_destroy(&buffers[bufref]->wlock);
//  free(buffers[bufref]->lock);

    free(buffers[bufref]);
    buffers[bufref] = NULL;

    sem_post(mod_storage);

    return 0;
}

int logger_buffer_add_message(int bufref, t_loggermsg* msg) {

    char* err_msg_hdr = (char*) "Can't add message to buffer:";
    if (_lgbf_check_values(bufref, err_msg_hdr))
        return 1;

    sem_wait(&buffers[bufref]->wlock); // get the lock

    // make sure there aren't too many messages on the buffer
    if ((BUFFER_SIZE - buffers[bufref]->amsgs) <= BUFFER_CLOSE_WARN) {
        fprintf(stderr, "%s there are too many unread messages.\n", err_msg_hdr);
        sem_post(&buffers[bufref]->wlock);
        return 1;
    }

    buffers[bufref]->messages[buffers[bufref]->awindx] = msg; // add the message
    buffers[bufref]->awindx = _lgbf_get_new_index(buffers[bufref]->awindx); // increment the write index
    buffers[bufref]->amsgs++; // increment the number of unread messages
    sem_post(&buffers[bufref]->wlock); // release the lock

    return 0;
}

/*
 * Removes a message from the buffer and returns a pointer to it.
 */
t_loggermsg* logger_buffer_read_message(int bufref) {

    char* err_msg_hdr = (char*) "Can't read message from buffer:";
    if (_lgbf_check_values(bufref, err_msg_hdr))
        return NULL;

    t_loggermsg* rtn_val = NULL;

    sem_wait(&buffers[bufref]->rlock); // get the lock

    if (buffers[bufref]->amsgs == 0) {
        fprintf(stderr, "%s there are no unread messages.\n", err_msg_hdr);
        sem_post(&buffers[bufref]->rlock);
        return NULL;
    }

    int read_index = buffers[bufref]->arindx; // get the current read index
    rtn_val = buffers[bufref]->messages[read_index]; // get the pointer to the message
    buffers[bufref]->messages[read_index] = NULL; // remove the message from the buffer
    buffers[bufref]->arindx = _lgbf_get_new_index(buffers[bufref]->arindx); // increment the read index
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
int logger_buffer_wait_for_messages(int bufref, int milliseconds_to_wait) {

    const int max_milliseconds_wait = 3000;
    const int min_milliseconds_wait = 1;
    char* err_msg_hdr = (char*) "Failed to wait for a message:";

    if (_lgbf_check_values(bufref, err_msg_hdr))
        return -1;

    // before doing anything else, check if there's already a message
    if (buffers[bufref]->amsgs > 0) {
        return 0;
    }

    // get the current time
    struct timespec break_time;
    if (clock_gettime(CLOCK_REALTIME, &break_time) == 1) {
        fprintf(stderr, "%s something went wrong getting the time\n", err_msg_hdr);
        return -1;
    }

    // determine the time to break from the loop
    logger_buffer_add_to_time(&break_time, milliseconds_to_wait, min_milliseconds_wait, max_milliseconds_wait);

    while(buffers[bufref]->amsgs == 0) {
        struct timespec loop_time;
        if (clock_gettime(CLOCK_REALTIME, &loop_time) == 1) {
            fprintf(stderr, "%s something went wrong getting the time\n", err_msg_hdr);
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

int logger_buffer_add_to_time(struct timespec *p_pTspec, int ms_to_add, int min_ms, int max_ms) {

    if (ms_to_add > max_ms) {
        fprintf(stderr, "Maximum time to wait is %d milliseconds.\n", max_ms);
        ms_to_add = max_ms;
    }
    else if (ms_to_add < min_ms) {
        fprintf(stderr, "Minimum time to wait is %d milliseconds.\n", min_ms);
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

#ifdef TESTING_FUNCTIONS_ENABLED
int logger_get_buffer_size() {
    return BUFFER_SIZE;
}

int logger_get_buffer_close_warn() {
    return BUFFER_CLOSE_WARN;
}
#endif

