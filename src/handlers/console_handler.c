
#include "console_handler.h"

#include <stdlib.h>
#include <string.h> //memcpy()

// global variables
typedef struct {
    FILE* m_pLog;
} consolehandler_data;

static const size_t g_sizeData = { sizeof(consolehandler_data) };

// private function declarations
static int _console_handler_close(log_handler *p_pHandler);
static int _console_handler_isOpen(const log_handler* p_pHandler);
static int _console_handler_open(log_handler* p_pHandler);
static int _console_handler_write(log_handler *p_pHandler, const t_loggermsg* p_sMsg);

// private function definitions
int _console_handler_close(log_handler *p_pHandler) {

    if (lgh_checks(p_pHandler, g_sizeData, "console handler"))
        return 1;

    ((consolehandler_data*)p_pHandler->m_pHandlerData)->m_pLog = NULL;
    free(p_pHandler->m_pHandlerData);
    p_pHandler->m_pHandlerData = NULL;
    return 0;
}

int _console_handler_isOpen(const log_handler *p_pHandler) {

    // return 1 so function passes if() checks
    if (lgh_checks(p_pHandler, g_sizeData, "console handler"))
        return 0;
    else return 1;
}

int _console_handler_open(log_handler *p_pHandler) {

    if (lgh_checks(p_pHandler, g_sizeData, "console handler"))
        return 1;

    return 0;
}

int _console_handler_write(log_handler *p_pHandler, const t_loggermsg* p_sMsg) {

    if (lgh_checks(p_pHandler, g_sizeData, "console handler"))
        return 1;
    else if (p_sMsg == NULL) {
        lgu_warn_msg("console handler given NULL message to log");
        return 1;
    }

    fprintf(
        ((consolehandler_data*)p_pHandler->m_pHandlerData)->m_pLog,
        "%s%s %s\n",
        p_sMsg->m_sFormat,
        p_sMsg->m_sId,
        p_sMsg->m_sMsg
    );
    fflush(((consolehandler_data*)p_pHandler->m_pHandlerData)->m_pLog);

    return 0;
}

// public function definitions
int create_console_handler(log_handler *p_pHandler, FILE *p_pOut) {

    if (p_pOut == NULL) {
        lgu_warn_msg("can't create console handler to NULL");
        return 1;
    }
    else if(p_pHandler == NULL) {
        lgu_warn_msg("can't store console handler in NULL ptr");
        return 1;
    }
    else if ((p_pOut != stdout ) && (p_pOut != stderr)) {
        lgu_warn_msg("console handler must be to stdout or stderr");
        return 1;
    }

    // allocate space for the handler data
    consolehandler_data* t_pData = (consolehandler_data*)malloc(sizeof(consolehandler_data));
    if (t_pData == NULL) {
        lgu_warn_msg("failed to allocate space for console handler data");
        return 1;
    }

    t_pData->m_pLog = p_pOut;

    log_handler t_structHandler = {
        &_console_handler_write,
        &_console_handler_close,
        &_console_handler_open,
        &_console_handler_isOpen,
        true,
        true,
        t_pData
    };

    memcpy(p_pHandler, &t_structHandler, sizeof(log_handler));

    return 0;
}

