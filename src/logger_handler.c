
#include "logger_handler.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h> //memset()

// public const definition
const t_handlerref CLOGGER_HANDLER_ERR = { (1<<1) | (1<<2) };

// file global vars
static log_handler** g_pHandlers = { NULL };
static sem_t*       g_pStorageSem = { NULL };
static atomic_bool  g_bInit = { false };
static atomic_int   g_nHandlers = { 0 };

static t_handlerref g_refValid = { 0 };

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

    // this should already be set
    g_refValid = 0;

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
    g_refValid = 0;

    sem_destroy(g_pStorageSem);
    free(g_pStorageSem);
    g_pStorageSem = NULL;

    return 0;
}

int lgh_checks(const log_handler *p_pHandler, __attribute__((unused))size_t p_sizeData, const char* p_sCaller) {
    if (p_pHandler == NULL) {
        lgu_warn_msg_str("%s function was not given a handler pointer", p_sCaller);
        return 1;
    }
    else if (p_pHandler->m_pHandlerData == NULL) {
        lgu_warn_msg_str("%s function given handler that doesn't have data stored", p_sCaller);
        return 1;
    }
    /*
     * FIXME check below doesn't work as intended; sizeof(*(void*)) returns 1
    else if (sizeof(*(p_pHandler->m_pHandlerData)) != p_sizeData) {
        lgu_warn_msg_str("%s function given handler with data of incorrect size", p_sCaller);
        return 1;
    }
    */

    return 0;
}

t_handlerref lgh_add_handler(const log_handler* p_pHandler) {

    if (_lgh_check_init()) {
        return CLOGGER_HANDLER_ERR;
    }

    sem_wait(g_pStorageSem); // get the storage lock

    t_handlerref t_refRtn = 0;

    for (int t_nHandlerIndex = 0; t_nHandlerIndex < CLOGGER_MAX_NUM_HANDLERS; t_nHandlerIndex++) {
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
            // mark the handler as valid in the global reference tracker
            t_refRtn = (1<<t_nHandlerIndex);
            g_refValid |= t_refRtn;
            break;
        }
    }

    sem_post(g_pStorageSem);

    if (t_refRtn == 0) return CLOGGER_HANDLER_ERR;
    else return t_refRtn;
}

int lgh_get_num_handlers() {

    if (_lgh_check_init()) {
        return 1;
    }

    int t_nHandlers = 0;

    sem_wait(g_pStorageSem);
    t_nHandlers = g_nHandlers;
    sem_post(g_pStorageSem);

    return t_nHandlers;
}

t_handlerref lgh_get_valid_handlers() {

    if (_lgh_check_init()) {
        return CLOGGER_HANDLER_ERR;
    }

    t_handlerref t_refRtn = 0;

    sem_wait(g_pStorageSem);
    t_refRtn = g_refValid;
    sem_post(g_pStorageSem);

    return t_refRtn;
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
            if (g_pHandlers[t_nCount]->isOpen(g_pHandlers[t_nCount])) {
                if (g_pHandlers[t_nCount]->close(g_pHandlers[t_nCount])) {
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
    // TODO in theory this function could support removing
    // multiple handlers at the same time, but for now we will
    // force it to only accept one
    if (_lgh_check_init()) {
        return 1;
    }
    else if (p_refIndex & ~g_refValid) {
        // the t_handlerref given isn't valid
        lgu_warn_msg("asked to remove a handler with an invalid reference");
    }

    sem_wait(g_pStorageSem); // get the storage lock
    for (t_handlerref t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if ((g_pHandlers[t_nCount] != NULL) && (p_refIndex & (1<<t_nCount))) {
            if (p_refIndex != (1<<t_nCount)) {
                // we were asked to remove more than one handler
                lgu_warn_msg("asked to remove more than one handler");
                sem_post(g_pStorageSem);
                return 1;
            }

            // check if the handler is open
            if (g_pHandlers[p_refIndex]->isOpen(g_pHandlers[t_nCount])) {
                // close the handler
                g_pHandlers[p_refIndex]->close(g_pHandlers[t_nCount]);
            }

            // free the memory for the handler
            free(g_pHandlers[t_nCount]);
            g_pHandlers[t_nCount] = NULL;
            g_nHandlers--;
            if (g_nHandlers < 0) g_nHandlers = 0;
            sem_post(g_pStorageSem);
            return 0;
        }
    }

    // don't think we can make it here
    sem_post(g_pStorageSem);

    return 1;
}

int lgh_write(t_handlerref p_refIndex, const t_loggermsg *p_pMsg) {
    if (_lgh_check_init()) {
        return 1;
    }
    else if (p_refIndex & ~g_refValid) {
        // the t_handlerref given isn't valid
        lgu_warn_msg("asked to write to a handler with an invalid reference");
        return 1;
    }

    int t_nRtn = 0;

    // TODO if each handler gets its own read/write lock, this won't be needed
    sem_wait(g_pStorageSem); // get the storage lock

    for (t_handlerref t_nCount = 0; t_nCount < CLOGGER_MAX_NUM_HANDLERS; t_nCount++) {
        if ((g_pHandlers[t_nCount] != NULL) && (p_refIndex & (1<<t_nCount))) {
            // check if the handler needs to be opened
            if (!g_pHandlers[t_nCount]->isOpen(g_pHandlers[t_nCount])) {
                // try to open the handler
                if (g_pHandlers[t_nCount]->open(g_pHandlers[t_nCount])) {
                    lgu_warn_msg("asked to write to a handler that isn't open, and it couldn't be opened");
                    t_nRtn++;
                    continue;
                }
            }
            // write the message to the handler
            if (g_pHandlers[t_nCount]->write(g_pHandlers[t_nCount], p_pMsg)) {
                // failed to write to a handler; indicate an error, but keep going
                t_nRtn++;
            }
        }
        if (p_refIndex <= (1<<t_nCount)) {
            // there aren't any higher bits set
            break;
        }
    }

    sem_post(g_pStorageSem);

    return t_nRtn;
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
            if (g_pHandlers[t_nCount]->isOpen(g_pHandlers[t_nCount])) {
                if (g_pHandlers[t_nCount]->write(g_pHandlers[t_nCount], p_pMsg)) {
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

