
#include "logger_id.h"
#include "logger_util.h" // lgu_warn_msg()

#include <semaphore.h>
#include <stdlib.h>
#include <string.h> // strlen()

#define LOGGER_ID_NUM_READERS   3

// global variables
static sem_t *g_pReadSem = { NULL };    // semaphore that allows reading values
static sem_t *g_pWriteSem = { NULL };   // semaphore that controls making modifications
static char** g_pIdValPtrs = { NULL };
static int volatile g_bRunning = { 0 };
static size_t g_nLongestLen = { 0 };

// private function declarations
static int _lgi_update_existing_vals(size_t new_len);

// private function definitions
int _lgi_update_existing_vals(size_t new_len) {
    /* this function expects the lock to already be required */
    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        if (g_pIdValPtrs[count] != NULL) {
            size_t len = strlen(g_pIdValPtrs[count]);
            int new_final_index = (int) ((len / sizeof(char)) + ((new_len - len) / sizeof(char)));
            int index;
            for (index = len; index < new_final_index; index++) {
                g_pIdValPtrs[count][index] = ' ';
            }
            g_pIdValPtrs[count][index] = '\0';
        }
    }
    return 0;
}

// public functions
int lgi_init() {

    if (g_bRunning) {
        lgu_warn_msg("Logger IDs have already been initialized.");
        return 1;
    }

    if (g_pIdValPtrs != NULL)
        return 1;

    g_pIdValPtrs = (char**) malloc(sizeof(char*) * LOGGER_ID_MAX_IDS);
    if (g_pIdValPtrs == NULL) {
        lgu_warn_msg("Failed to allocate space for logger IDs.");
        return 1;
    }

    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        g_pIdValPtrs[count] = NULL;
    }

    g_pReadSem = (sem_t*) malloc(sizeof(sem_t));
    g_pWriteSem = (sem_t*) malloc(sizeof(sem_t));
    if ((g_pReadSem == NULL) || (g_pWriteSem == NULL)) {
        lgu_warn_msg("Failed to allocate space for logger id locks.");
        return 1;
    }
    sem_init(g_pReadSem, 0, LOGGER_ID_NUM_READERS);
    sem_init(g_pWriteSem, 0, 1);

    g_bRunning = 1;

    return 0;
}

int lgi_free() {

    if (!g_bRunning) {
        lgu_warn_msg("Logger IDs were not initialized.");
        // TODO is this an error?
        return 0;
    }

    g_bRunning = 0;

    if (g_pIdValPtrs == NULL) {
        // TODO should we indicate an error?
        return 0;
    }

    // get the mutex to modify data
    sem_wait(g_pWriteSem);
    // get all of the reader locks
    for (int count = 0; count < LOGGER_ID_NUM_READERS; count++) {
        // TODO change to sem_timedwait()
        sem_wait(g_pReadSem);
    }

    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        if (g_pIdValPtrs[count] != NULL) {
            free(g_pIdValPtrs[count]);
            g_pIdValPtrs[count] = NULL;
        }
    }
    free(g_pIdValPtrs);
    g_pIdValPtrs = NULL;

    sem_destroy(g_pReadSem);
    free(g_pReadSem);

    sem_destroy(g_pWriteSem);
    free(g_pWriteSem);

    g_pReadSem = NULL;
    g_pWriteSem = NULL;

    g_nLongestLen = 0;

    return 0;
}

logger_id lgi_add_id(char* p_sId) {

    if (!g_bRunning) {
        lgu_warn_msg("Logger IDs not initialized.");
        return -1;
    }

    int rtn_val = -1;

    // get the mutex to modify data
    sem_wait(g_pWriteSem);
    // get all of the reader locks
    for (int count = 0; count < LOGGER_ID_NUM_READERS; count++) {
        // TODO change to sem_timedwait()
        sem_wait(g_pReadSem);
    }

    // have all the locks; modify the data
    for (int count = 0; count < LOGGER_ID_MAX_IDS; count++) {
        if (g_pIdValPtrs[count] == NULL) {
            g_pIdValPtrs[count] = (char*) malloc(sizeof(char) * CLOGGER_ID_MAX_LEN);
            if (g_pIdValPtrs[count] == NULL) {
                lgu_warn_msg("Failed to create space for the new logger ID.");
                break;
            }
            size_t snrtn = snprintf(g_pIdValPtrs[count], CLOGGER_ID_MAX_LEN, "%s", p_sId);
            if (snrtn >= CLOGGER_ID_MAX_LEN) {
                lgu_warn_msg("The logger ID given was too large to add.");
                break;
            }

            if (snrtn > g_nLongestLen) {
                g_nLongestLen = snrtn;
                _lgi_update_existing_vals(g_nLongestLen);
            }
            rtn_val = count;
            break;
        }
    }

    for (int sem_count = 0; sem_count < LOGGER_ID_NUM_READERS; sem_count++) {
        sem_post(g_pReadSem);
    }
    sem_post(g_pWriteSem);

    return rtn_val;
}

int lgi_get_id(logger_id id_ref, char* dest) {

    if (!g_bRunning) {
        lgu_warn_msg("Logger IDs not initialized.");
        return 1;
    }

    int rtn_val = 0;

    // TODO sem_trywait() or sem_timedwait()
    sem_wait(g_pReadSem);
    if (g_pIdValPtrs[id_ref] == NULL) {
        lgu_warn_msg_int("No value exists for ID at location %d", id_ref);
        rtn_val = 1;
    }
    else if (dest == NULL) {
        lgu_warn_msg("Can't copy ID to NULL location.");
        rtn_val = 1;
    }

    if (rtn_val == 0) {
        int copy_rtn = snprintf(dest, CLOGGER_ID_MAX_LEN, "%s", g_pIdValPtrs[id_ref]);
        if ((copy_rtn >= CLOGGER_ID_MAX_LEN) || (copy_rtn < 0)) {
            lgu_warn_msg("Failed to copy the ID to the destination.");
            rtn_val = 1;
        }
    }

    sem_post(g_pReadSem);
    return rtn_val;

}

int lgi_get_longest_len() {
    return g_nLongestLen;
}

int lgi_remove_id(logger_id id_ref) {

    if (!g_bRunning) {
        return -1;
    }

    // don't remove the default id; it will be manually freed
    if (id_ref == CLOGGER_DEFAULT_ID)
        return -1;

    int rtn_val = 1;

    // get the mutex to modify data
    sem_wait(g_pWriteSem);
    // get all of the reader locks
    for (int count = 0; count < LOGGER_ID_NUM_READERS; count++) {
        // TODO change to sem_timedwait()
        sem_wait(g_pReadSem);
    }

    // have all the locks; modify the data
    if (g_pIdValPtrs[id_ref] == NULL) {
        lgu_warn_msg_int("No ID to remove at %d", id_ref);
        rtn_val = 1;
    }
    else {
        free(g_pIdValPtrs[id_ref]);
        g_pIdValPtrs[id_ref] = NULL;
        rtn_val = 0;
    }

    for (int sem_count = 0; sem_count < LOGGER_ID_NUM_READERS; sem_count++) {
        sem_post(g_pReadSem);
    }

    // TODO determine length of item removed and update g_nLongestLen ?
    sem_post(g_pWriteSem);

    return rtn_val;
}

