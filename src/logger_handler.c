
#include "logger_handler.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h> //memset()

// file global vars
static log_handler** g_pHandlers = { NULL };
static sem_t*       g_pStorageSem = { NULL };
static atomic_bool  g_bInit = { false };
static atomic_int   g_nHandlers = { 0 };

// private function declarations
int _lgh_check_init();

// private function definitions
int _lgh_check_init() {
    char* t_sErrMsg = NULL;
    if (!g_bInit) t_sErrMsg = "handlers not initialized";
    else if (g_pHandlers == NULL) t_sErrMsg = "handlers not allocated";
    else if (g_pStorageSem == NULL) t_sErrMsg = "handler storage lock doesn't exist";

    if (t_sErrMsg != NULL) {
        lgu_warn_msg(t_sErrMsg);
        return 1;
    }

    return 0;
}

// public functions
int lgh_init() {

    if ((g_pHandlers != NULL) || (g_pStorageSem != NULL)) {
        lgu_warn_msg("handlers have already been initialized");
        return 1;
    }

    // allocate space for the handlers
    g_pHandlers = (log_handler**)malloc(sizeof(log_handler*) * CLOGGER_MAX_NUM_HANDLERS);
    if (g_pHandlers == NULL) {
        lgu_warn_msg("failed to allocate space for the handlers");
        return 1;
    }

    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        g_pHandlers[t_nCount] = NULL;
    }

    // allocate space for the semaphore
    g_pStorageSem = (sem_t*)malloc(sizeof(sem_t));
    if (g_pStorageSem == NULL) {
        lgu_warn_msg("failed to allocate space for the handler lock");
        return 1;
    }
    g_nHandlers = 0;
    sem_init(g_pStorageSem, 0, 1);

    g_bInit = true;

    return 0;
}

int lgh_free() {

    g_bInit = false;

    int t_nRtn = 0;
    if (g_pStorageSem == NULL) {
        lgu_warn_msg("handler lock doesn't exist");
        t_nRtn++;
    }
    else {
        // TODO might not need this if we implement individual handler free function
        sem_wait(g_pStorageSem);
    }

    if (g_pHandlers == NULL) {
        lgu_warn_msg("no handlers to free");
        t_nRtn++;
    }
    else {
        for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
            if (g_pHandlers[t_nCount] != NULL) {
                free(g_pHandlers[t_nCount]);
            }
        }
        free(g_pHandlers);
        g_pHandlers = NULL;
    }

    g_nHandlers = 0;

    sem_destroy(g_pStorageSem);
    free(g_pStorageSem);
    g_pStorageSem = NULL;

    return 0;
}

int lgh_add_handler(const log_handler* p_pHandler) {

    if (_lgh_check_init()) {
        return -1;
    }

    int t_nHandlerIndex = 0;
    sem_wait(g_pStorageSem); // get the storage lock

    for (t_nHandlerIndex = 0; t_nHandlerIndex < CLOGGER_MAX_NUM_HANDLERS; t_nHandlerIndex++) {
        // look for empty space for the handler
        if (g_pHandlers[t_nHandlerIndex] == NULL) {
            // allocate space for the new handler
            g_pHandlers[t_nHandlerIndex] = (log_handler*)malloc(sizeof(log_handler));
            if (g_pHandlers[t_nHandlerIndex] == NULL) {
                lgu_warn_msg("failed to allocate space for new handler");
                break;
            }
            // use memcpy to set due to const ptrs
            memcpy(g_pHandlers[t_nHandlerIndex], p_pHandler, sizeof(log_handler));
            g_nHandlers++;
            break;
        }
    }

    sem_post(g_pStorageSem);

    if (t_nHandlerIndex >= CLOGGER_MAX_NUM_HANDLERS) return -1;
    else return t_nHandlerIndex;
}

int lgh_get_num_handlers() {
    return g_nHandlers;
}

int lgh_open_handlers() {
    if (_lgh_check_init()) {
        return 1;
    }

    int t_nRtn = 0;

    // TODO this code might not be fully thread-safe, but we're going to check
    // for handlers that aren't open before grabbing the lock
    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if (g_pHandlers[t_nCount] != NULL) {
            if (!g_pHandlers[t_nCount]->isOpen()) {
                // now grab the lock TODO might be too late for thread safety
                sem_wait(g_pStorageSem); // get the storage lock
                if (g_pHandlers[t_nCount]->open()) {
                    // failed to open a handler
                    t_nRtn++;
                }
                /*
                else {
                    g_nHandlers++;
                }
                */
                sem_post(g_pStorageSem);
            }
        }
    }

    return t_nRtn;

}

int lgh_remove_all_handlers() {
    if (_lgh_check_init()) {
        return 1;
    }

    // if there aren't any handlers, nothing to do
    if (g_nHandlers == 0) return 0;

    int t_nRtn = 0;
    sem_wait(g_pStorageSem); // get the storage lock

    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if (g_pHandlers[t_nCount] != NULL) {
            if (g_pHandlers[t_nCount]->isOpen()) {
                if (g_pHandlers[t_nCount]->close()) {
                    // failed to close a handler
                    t_nRtn++;
                }
            }
            free(g_pHandlers[t_nCount]);
            g_pHandlers[t_nCount] = NULL;
            g_nHandlers--;
        }
    }

    sem_post(g_pStorageSem);

    return t_nRtn;
}

int lgh_remove_handler(t_handlerref p_refIndex) {
    if (_lgh_check_init()) {
        return 1;
    }
    else if (p_refIndex >= CLOGGER_MAX_NUM_HANDLERS) {
        lgu_warn_msg("index of handler to write to is too large");
        return 1;
    }
    else if(g_pHandlers[p_refIndex] == NULL) {
        lgu_warn_msg("can't remove handler that hasn't been set");
        return 1;
    }

    // TODO if each handler gets its own read/write lock, this won't be needed
    sem_wait(g_pStorageSem); // get the storage lock
    if (g_pHandlers[p_refIndex]->isOpen()) {
        // close the handler
        g_pHandlers[p_refIndex]->close();
    }
    // free the memory for the handler
    free(g_pHandlers[p_refIndex]);
    g_pHandlers[p_refIndex] = NULL;
    g_nHandlers--;
    if (g_nHandlers < 0) g_nHandlers = 0;
    sem_post(g_pStorageSem);

    return 0;
}

int lgh_write(t_handlerref p_refIndex, const t_loggermsg *p_pMsg) {
    if (_lgh_check_init()) {
        return 1;
    }
    else if (p_refIndex >= CLOGGER_MAX_NUM_HANDLERS) {
        lgu_warn_msg("index of handler to write to is too large");
        return 1;
    }
    else if(g_pHandlers[p_refIndex] == NULL) {
        lgu_warn_msg("can't write to handler that hasn't been set");
        return 1;
    }

    // TODO if each handler gets its own read/write lock, this won't be needed
    sem_wait(g_pStorageSem); // get the storage lock

    g_pHandlers[p_refIndex]->write(p_pMsg);

    sem_post(g_pStorageSem);

    return 0;
}

int lgh_write_to_all(const t_loggermsg *p_pMsg) {
    if (_lgh_check_init()) {
        return 1;
    }

    int t_nHandlersWritten = 0;

    // TODO if each handler gets its own read/write lock, this won't be needed
    sem_wait(g_pStorageSem); // get the storage lock

    for (int t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if (g_pHandlers[t_nCount] != NULL) {
            if (g_pHandlers[t_nCount]->isOpen()) {
                if (g_pHandlers[t_nCount]->write(p_pMsg)) {
                    // failed to write to a handler
                    // TODO could we identify the handler that failed?
                    lgu_warn_msg_int("failed to write to open handler at reference %d", t_nCount);
                }
                else {
                    t_nHandlersWritten++;
                }
            }
        }
    }

    sem_post(g_pStorageSem);

    if (t_nHandlersWritten) return 0;
    else return 1;
}

