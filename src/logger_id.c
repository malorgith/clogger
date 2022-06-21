
#include "logger_id.h"
#include "logger_util.h" // lgu_warn_msg()

#include <stdatomic.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h> // strlen()

// global variables
static int volatile g_bRunning = { 0 };
static int g_nLongestLen = { 0 };

typedef struct {
    atomic_int      m_nWaitingMessages;
    bool            m_bAcceptMsgs;
    char            m_sId[CLOGGER_ID_MAX_LEN];
    sem_t           m_semLock;
    t_handlerref    m_refHandlers;
} loggerid_struct;

/*
 * TODO
 * Move m_nWaitingMessages out of the struct
 * and create a global variable array of atomic_ints
 * that serves the same purpose. This will reduce
 * the number of checks and locks that will be required
 * before increasing or decreasing the value.
 */

/* TODO
 * MAYBE
 * We could move the calls for the handlers to write from
 * logger.c into here.
 * Potential downside of holding on to the logger_id locks longer,
 * but maybe we reduce the possibility of having a handler removed
 * while we're attempting to log to it?
 * With the m_nWaitingMessages moved to global array it wouldn't be
 * as bad for us to hold on to the lock for a bit, maybe.
 */

loggerid_struct   **g_pIds = { NULL };
static sem_t*       g_pStorageSem = { NULL };

// private function declarations
static int _lgi_checks(logger_id, const char*);
static int _lgi_update_existing_vals(size_t new_len);
static int _lgi_remove_all_messages(logger_id);

// private function definitions
/*
 * The function below checks the sanity of the global variables,
 * makes sure we're running, and _GRABS THE LOCK_ on the id
 * specified at the reference.
 */
static int _lgi_checks(logger_id p_refId, const char *p_sErrMsg) {
    if (!g_bRunning) {
        lgu_warn_msg_str("%s: logger ids aren't running", p_sErrMsg);
        return 1;
    }
    else if (g_pIds == NULL) {
        lgu_warn_msg_str("%s: space for logger ids hasn't been allocated", p_sErrMsg);
        return 1;
    }
    else if (g_pIds[p_refId] == NULL) {
        lgu_warn_msg_str("%s: id reference given points to null", p_sErrMsg);
        return 1;
    }

    // get the lock on the id
    // TODO timedwait ?
    sem_wait(&g_pIds[p_refId]->m_semLock);

    return 0;
}

int _lgi_remove_all_messages(logger_id p_refId) {

    // do sanity checks and get object's lock
    if (_lgi_checks(p_refId, "can't clear waiting messages"))
        return 1;

    g_pIds[p_refId]->m_nWaitingMessages = 0;
    sem_post(&g_pIds[p_refId]->m_semLock);

    return 0;
}

int _lgi_update_existing_vals(size_t new_len) {
    /* this function expects the storage lock to already be required */
    for (int count = 0; count < CLOGGER_MAX_NUM_IDS; count++) {
        if (g_pIds[count] != NULL) {
            // do sanity check and get id's lock
            if (_lgi_checks(count, "can't update existing id"))
                return 1;
            size_t len = strlen(g_pIds[count]->m_sId);
            int new_final_index = (int) ((len / sizeof(char)) + ((new_len - len) / sizeof(char)));
            int index;
            for (index = len; index < new_final_index; index++) {
                g_pIds[count]->m_sId[index] = ' ';
            }
            g_pIds[count]->m_sId[index] = '\0';

            // free the object's lock
            sem_post(&g_pIds[count]->m_semLock);
        }
    }
    return 0;
}

// public functions
int lgi_init() {

    if (g_bRunning) {
        lgu_warn_msg("log ids already initialized");
        return 1;
    }
    else if (g_pIds != NULL) {
        lgu_warn_msg("log ids aren't running, but there's data stored");
        return 1;
    }

    // allocate space to store the IDs
    g_pIds = (loggerid_struct**) malloc(sizeof(loggerid_struct*) * CLOGGER_MAX_NUM_IDS);
    if (g_pIds == NULL) {
        lgu_warn_msg("failed to allocate space to store logger ids");
        return 1;
    }

    // init each id location to NULL
    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_IDS; t_nCount++) {
        g_pIds[t_nCount] = NULL;
    }

    g_pStorageSem = (sem_t*) malloc(sizeof(sem_t));

    if (g_pStorageSem == NULL) {
        lgu_warn_msg("logger id failed to allocate space for storage lock");
        free(g_pIds);
        g_pIds = NULL;
        return 1;
    }
    sem_init(g_pStorageSem, 0, 1);

    g_bRunning = 1;

    return 0;
}

int lgi_free() {

    if (!g_bRunning) {
        lgu_warn_msg("asked to free logger ids, but they weren't initialized");
        // TODO is this an error?
        return 0;
    }
    else if (g_pIds == NULL) {
        lgu_warn_msg("asked to free logger ids, and we thought we're running, but ids are null");
        // this shouldn't happen, so it's an error
        return 1;
    }

    // TODO it would be good to set g_bRunning = 0 here, but lgi_remove_id() will
    // get upset (through _lgi_checks()) if we aren't running

    /*
     * we can't get the storage lock yet because we get that in lgi_remove_id(),
     * but by setting g_bRunning to false we should ensure no new messages or ids
     * come in
     */

    // remove all active ids
    for (logger_id t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_IDS; t_nCount++) {
        if (g_pIds[t_nCount]) {
        /*
         * This function is called after the logger has already started its shutdown process.
         * Clear the value of m_nWaitingMessages for each ID since no more messages will be
         * written.
         */
            _lgi_remove_all_messages(t_nCount);
            lgi_remove_id(t_nCount);
        }
    }

    g_bRunning = 0;

    // get the storage lock
    // TODO timedwait ?
    sem_wait(g_pStorageSem);

    free(g_pIds);
    g_pIds = NULL;

    g_nLongestLen = 0;

    sem_destroy(g_pStorageSem);
    free(g_pStorageSem);
    g_pStorageSem = NULL;
    return 0;
}

int lgi_add_handler(logger_id p_refId, t_handlerref p_refHandler) {

    // do sanity checks and get object's lock
    if (_lgi_checks(p_refId, "can't add handler"))
        return 1;
    // TODO should we check if it's still accepting messages? adding a handler won't keep it from freeing

    g_pIds[p_refId]->m_refHandlers |= p_refHandler;

    // TODO any sort of checks we can do? we don't know the valid handlers here

    sem_post(&g_pIds[p_refId]->m_semLock);

    return 0;
}

logger_id lgi_add_id(const char* p_sId) {
    return lgi_add_id_w_handler(p_sId, 0);
}

logger_id lgi_add_id_w_handler(const char* p_sId, t_handlerref p_refHandlers) {

    if (!g_bRunning) {
        lgu_warn_msg("asked to add an id, but we're not running");
        return CLOGGER_MAX_NUM_IDS;
    }
    else if (g_pIds == NULL) {
        lgu_warn_msg("asked to add an id, and think we're running, but no space allocated for ids");
        return CLOGGER_MAX_NUM_IDS;
    }

    // TODO timedwait?
    sem_wait(g_pStorageSem);

    // find an empty slot for the id
    for (logger_id t_refId = 0; t_refId < CLOGGER_MAX_NUM_IDS; t_refId++) {
        if (g_pIds[t_refId] == NULL) {
            // allocate space for the ID
            g_pIds[t_refId] = (loggerid_struct*) malloc(sizeof(loggerid_struct));
            if (g_pIds[t_refId] == NULL) {
                lgu_warn_msg("failed to allocate space for a new id");
                sem_post(g_pStorageSem);
                return CLOGGER_MAX_NUM_IDS;
            }

            // store the string given in the struct
            static const char* t_sErrMsg = "failed to store the id given";
            if (lgu_wsnprintf(g_pIds[t_refId]->m_sId, CLOGGER_ID_MAX_LEN, t_sErrMsg, "%s", p_sId)) {
                // failed to store the string given; free resources and fail
                free(g_pIds[t_refId]);
                sem_post(g_pStorageSem);
                return CLOGGER_MAX_NUM_IDS;
            }

            // init other values in struct
            g_pIds[t_refId]->m_nWaitingMessages = 0;
            g_pIds[t_refId]->m_bAcceptMsgs = true;
            g_pIds[t_refId]->m_refHandlers = p_refHandlers;
            sem_init(&g_pIds[t_refId]->m_semLock, 0, 1);

            // check if we need to add space padding to existing values
            /*** the new id's lock must be initialized before we do this ***/
            int t_nLen = strlen(g_pIds[t_refId]->m_sId);
            if (t_nLen > g_nLongestLen) {
                // the new ID is longer than any previous one; space pad them to match
                g_nLongestLen = t_nLen;
                _lgi_update_existing_vals(g_nLongestLen);
            }

            // nothing else to do; free lock
            sem_post(g_pStorageSem);

            return t_refId;
        }
    }

    // if we're here we didn't find an empty spot in the loop above
    sem_post(g_pStorageSem);
    lgu_warn_msg("asked to create a new log id, but there isn't space for additional ids");
    return CLOGGER_MAX_NUM_IDS;
}

int lgi_add_message(logger_id p_refId) {

    // do sanity checks and get object's lock
    if (_lgi_checks(p_refId, "can't increase message count"))
        return 1;
    else if (!g_pIds[p_refId]->m_bAcceptMsgs) {
        lgu_warn_msg("asked to add message to ID that isn't accepting them");
        sem_post(&g_pIds[p_refId]->m_semLock);
        return 1;
    }

    g_pIds[p_refId]->m_nWaitingMessages++;
    sem_post(&g_pIds[p_refId]->m_semLock);

    return 0;
}

/*
 * This function retrieves the string value of the specified ID
 * and stores it in 'dest'.
 *
 * On success, returns the ID's m_refHandlers to tell the caller which
 * handlers should be written to. Returns 0, which isn't a valid
 * t_handlerref, on failure.
 */
t_handlerref lgi_get_id(logger_id p_refId, char* dest) {

    static const char *t_sErrMsg = "failed to copy id to destination";
    t_handlerref t_refRtn = CLOGGER_HANDLER_ERR;

    if (dest == NULL) {
        lgu_warn_msg("can't copy id to a null pointer");
        return 0;
    }
    else if (_lgi_checks(p_refId, "can't get id"))
        return 0;

    if (!lgu_wsnprintf(dest, CLOGGER_ID_MAX_LEN, t_sErrMsg, "%s", g_pIds[p_refId]->m_sId)) {
        // no error; update the return value while we have the lock
        t_refRtn = g_pIds[p_refId]->m_refHandlers;
    }

    sem_post(&g_pIds[p_refId]->m_semLock);

    return t_refRtn;

}

int lgi_remove_handler(logger_id p_refId, t_handlerref p_refHandler) {

    // do sanity check and get id's lock
    if (_lgi_checks(p_refId, "can't remove id"))
        return 1;
    // TODO should we check if it's still accepting messages? adding a handler won't keep it from freeing

    g_pIds[p_refId]->m_refHandlers &= ~p_refHandler;

    // TODO any sort of checks we can do? we don't know the valid handlers here

    sem_post(&g_pIds[p_refId]->m_semLock);

    return 0;
}

int lgi_remove_id(logger_id p_refId) {

    // do sanity check and get id's lock
    if (_lgi_checks(p_refId, "can't remove id"))
        return 1;

    // tell the id to stop accepting new messages
    g_pIds[p_refId]->m_bAcceptMsgs = false;

    // release the lock on the id that was acquired in _lgi_checks()
    sem_post(&g_pIds[p_refId]->m_semLock);

    // now get the storage lock
    // TODO timedwait ?
    sem_wait(g_pStorageSem);

    // TODO should we sleep or anything? might not be smart since
    // we currently have a lock
    while ((g_pIds[p_refId] != NULL) && (g_pIds[p_refId]->m_nWaitingMessages > 0)) {
        // TODO make sure this can't cause an infinite loop somehow...
        continue;
    }

    // there aren't any messages waiting to be processed using the id
    // TODO timedwait ?
    sem_wait(&g_pIds[p_refId]->m_semLock);
    sem_destroy(&g_pIds[p_refId]->m_semLock);

    free(g_pIds[p_refId]);
    g_pIds[p_refId] = NULL;

    sem_post(g_pStorageSem);

    return 0;
}

int lgi_remove_message(logger_id p_refId) {

    // do sanity checks and get object's lock
    if (_lgi_checks(p_refId, "can't remove a message"))
        return 1;
    else if (g_pIds[p_refId]->m_nWaitingMessages < 1) {
        lgu_warn_msg("asked to decrease number of messages on ID that has less than one message");
        g_pIds[p_refId]->m_nWaitingMessages = 0;
        sem_post(&g_pIds[p_refId]->m_semLock);
        return 1;
    }

    g_pIds[p_refId]->m_nWaitingMessages--;
    sem_post(&g_pIds[p_refId]->m_semLock);

    return 0;
}

