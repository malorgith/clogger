
#include "console_handler.h"

#include <string.h> //memcpy()

// GLOBAL VARS
static FILE* g_pOut = { NULL };
// END GLOBAL VARS

// PRIVATE FUNCTION DECLARATIONS
static int _console_handler_close();
static int _console_handler_write(const t_loggermsg* p_sMsg);
// END PRIVATE FUNCTION DECLARATIONS

// PRIVATE FUNCTION DEFINITIONS
int _console_handler_close() {
    // nothing to do for the console
    return 0;
}

int _console_handler_write(const t_loggermsg* p_sMsg) {
    // TODO check a status or anything?
    fprintf(g_pOut, "%s%s %s\n", p_sMsg->m_sFormat, p_sMsg->m_sId, p_sMsg->m_sMsg);
    return 0;
}

int _console_handler_open() {
    // TODO should we do some checks here?
    return 0;
}

int _console_handler_isOpen() {
    // TODO should we do some checks here?
    // return 1 so function passes if() checks
    return 1;
}
// END PRIVATE FUNCTION DEFINITIONS

// PUBLIC FUNCTION DEFINITIONS
int create_console_handler(log_handler *p_pHandler, FILE *p_pOut) {

    if (p_pOut == NULL) {
        fprintf(stderr, "console_handler: Can't output to NULL\n");
        return 1;
    }
    else if(p_pHandler == NULL) {
        fprintf(stderr, "console_handler: handler pointer cannot be NULL\n");
        return 1;
    }
    else if ((p_pOut != stdout ) && (p_pOut != stderr)) {
        fprintf(stderr, "console_handler: The output must be either stdout or stderr.\n");
        return 1;
    }

    g_pOut = p_pOut;

    log_handler t_structHandler = {
        &_console_handler_write,
        &_console_handler_close,
        &_console_handler_open,
        &_console_handler_isOpen,
        true,
        true
    };

    memcpy(p_pHandler, &t_structHandler, sizeof(log_handler));

    return 0;
}
// END PUBLIC FUNCTION DEFINITIONS
