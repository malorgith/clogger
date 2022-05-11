
#include "logger_id.h"

#include <semaphore.h>
#include <stdlib.h>
#include <string.h> // strlen()

#define LOGGER_ID_NUM_READERS   3

// GLOBAL VARS
static sem_t *logger_id_read_sem = { NULL };    // semaphore that allows reading values
static sem_t *logger_id_write_sem = { NULL };   // semaphore that controls making modifications
static char** logger_id_values = { NULL };
static int volatile logger_id_run = { 0 };
static size_t logger_id_longest_len = { 0 };
// END GLOBAL VARS


// PRIVATE FUNCTION DECLARATIONS
static int _logger_id_update_existing_vals(size_t new_len);
// END PRIVATE FUNCTION DECLARATIONS


// PRIVATE FUNCTION DEFINITIONS

// END PRIVATE FUNCTION DEFINITIONS

int logger_id_init() {

    if (logger_id_run) {
        fprintf(stderr, "Logger IDs have already been initialized.\n");
        return 1;
    }

    if (logger_id_values != NULL)
        return 1;

    logger_id_values = (char**) malloc(sizeof(char*) * LOGGER_ID_MAX_IDS);
    if (logger_id_values == NULL) {
        fprintf(stderr, "Failed to allocate space for logger IDs.\n");
        return 1;
    }

    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        logger_id_values[count] = NULL;
    }

    logger_id_read_sem = (sem_t*) malloc(sizeof(sem_t));
    logger_id_write_sem = (sem_t*) malloc(sizeof(sem_t));
    if ((logger_id_read_sem == NULL) || (logger_id_write_sem == NULL)) {
        fprintf(stderr, "Failed to allocate space for logger id locks.\n");
        return 1;
    }
    sem_init(logger_id_read_sem, 0, LOGGER_ID_NUM_READERS);
    sem_init(logger_id_write_sem, 0, 1);

    logger_id_run = 1;

    return 0;
}

int logger_id_free() {

    if (!logger_id_run) {
        fprintf(stderr, "Logger IDs were not initialized.\n");
        // TODO is this an error?
        return 0;
    }

    logger_id_run = 0;

    // FIXME need to get semaphores before doing this

    if (logger_id_values == NULL) {
        // TODO should we indicate an error?
        return 0;
    }

    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        if (logger_id_values[count] != NULL) {
            free(logger_id_values[count]);
        }
        logger_id_values[count] = NULL;
    }
    free(logger_id_values);
    logger_id_values = NULL;

    sem_destroy(logger_id_read_sem);
    free(logger_id_read_sem);

    sem_destroy(logger_id_write_sem);
    free(logger_id_write_sem);

    logger_id_read_sem = NULL;
    logger_id_write_sem = NULL;

    logger_id_longest_len = 0;

    return 0;
}

logger_id logger_id_add_id(char* p_sId) {

    if (!logger_id_run) {
        fprintf(stderr, "Logger IDs not initialized.\n");
        return -1;
    }

    int rtn_val = -1;

    // get the mutex to modify data
    sem_wait(logger_id_write_sem);
    // get all of the reader locks
    for (int count = 0; count < LOGGER_ID_NUM_READERS; count++) {
        // TODO change to sem_timedwait()
        sem_wait(logger_id_read_sem);
    }

    // have all the locks; modify the data
    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        if (logger_id_values[count] == NULL) {
            logger_id_values[count] = (char*) malloc(sizeof(char) * LOGGER_ID_MAX_LEN);
            if (logger_id_values[count] == NULL) {
                fprintf(stderr, "Failed to create space for the new logger ID.\n");
                break;
            }
            size_t snrtn = snprintf(logger_id_values[count], LOGGER_ID_MAX_LEN, "%s", p_sId);
            if (snrtn >= LOGGER_ID_MAX_LEN) {
                fprintf(stderr, "The logger ID given was too large to add.\n");
                break;
            }

            if (snrtn > logger_id_longest_len) {
                logger_id_longest_len = snrtn;
                _logger_id_update_existing_vals(logger_id_longest_len);
            }
            rtn_val = count;
            break;
        }
    }

    for (int sem_count = 0; sem_count < LOGGER_ID_NUM_READERS; sem_count++) {
        sem_post(logger_id_read_sem);
    }
    sem_post(logger_id_write_sem);

    return rtn_val;
}

int _logger_id_update_existing_vals(size_t new_len) {
    /* XXX EXPECTS LOCK TO ALREADY BE ACQUIRED XXX */
    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        if (logger_id_values[count] != NULL) {
            size_t len = strlen(logger_id_values[count]);
            int new_final_index = (int) ((len / sizeof(char)) + ((new_len - len) / sizeof(char)));
            int index;
            for (index = len; index < new_final_index; index++) {
                logger_id_values[count][index] = ' ';
            }
            logger_id_values[count][index] = '\0';
            /*
            int indx = len;
            if (len < new_len) {
                int spaces_added = 0;
                while((spaces_added < (new_len - len)) && ((len + spaces_added) < LOGGER_ID_MAX_LEN)) {
                    logger_id_values[count][indx] = ' ';
                    indx++;
                    spaces_added++;
                }
                logger_id_values[count][indx] = '\0';
            }
            */
        }
    }
    return 0;
}

int logger_id_get_id(logger_id id_ref, char* dest) {

    if (!logger_id_run) {
        fprintf(stderr, "Logger IDs not initialized.\n");
        return 1;
    }

    int rtn_val = 0;

    // TODO sem_trywait() or sem_timedwait()
    sem_wait(logger_id_read_sem);
    if (logger_id_values[id_ref] == NULL) {
        fprintf(stderr, "No value exists for ID at location %d\n", id_ref);
        rtn_val = 1;
    }
    else if (dest == NULL) {
        fprintf(stderr, "Can't copy ID to NULL location.\n");
        rtn_val = 1;
    }

    if (rtn_val == 0) {
        int copy_rtn = snprintf(dest, LOGGER_ID_MAX_LEN, "%s", logger_id_values[id_ref]);
        if ((copy_rtn >= LOGGER_ID_MAX_LEN) || (copy_rtn < 0)) {
            fprintf(stderr, "Failed to copy the ID to the destination.\n");
            rtn_val = 1;
        }
    }

    sem_post(logger_id_read_sem);
    return rtn_val;

}

int logger_id_get_longest_len() {
    return logger_id_longest_len;
}

int logger_id_remove_id(logger_id id_ref) {

    if (!logger_id_run) {
//      fprintf(stderr, "Logger IDs not initialized.\n");
        return -1;
    }

    int rtn_val = 1;

    // get the mutex to modify data
    sem_wait(logger_id_write_sem);
    // get all of the reader locks
    for (int count = 0; count < LOGGER_ID_NUM_READERS; count++) {
        // TODO change to sem_timedwait()
        sem_wait(logger_id_read_sem);
    }

    // have all the locks; modify the data
    if (logger_id_values[id_ref] == NULL) {
        fprintf(stderr, "No ID to remove at %d\n", id_ref);
        rtn_val = 1;
    }
    else {
        free(logger_id_values[id_ref]);
        logger_id_values[id_ref] = NULL;
        rtn_val = 0;
    }

    for (int sem_count = 0; sem_count < LOGGER_ID_NUM_READERS; sem_count++) {
        sem_post(logger_id_read_sem);
    }

    // TODO determine length of item removed and update logger_id_longest_len
    sem_post(logger_id_write_sem);

    return rtn_val;
}
