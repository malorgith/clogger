
#include "file_handler.h"

#include "../logger_util.h"

#include <string.h> // memcpy()
#include <sys/stat.h>   // mkdir()

#define MAX_LEN_FILE_W_PATH 75

// GLOBAL VARS
static FILE* g_fpLog = { NULL };
static char g_sFileWithPath[MAX_LEN_FILE_W_PATH]; // FIXME make macro/defined
// END GLOBAL VARS

// PRIVATE FUNCTION DECLARATIONS
static int _file_handler_close();
static int _file_handler_write(const t_loggermsg* p_sMsg);
// END PRIVATE FUNCTION DECLARATIONS

// PRIVATE FUNCTION DEFINITIONS
int _file_handler_close() {

    if (g_fpLog == NULL) {
        return 1;
    }
    fclose(g_fpLog);
    g_fpLog = NULL;

    return 0;
}

int _file_handler_write(const t_loggermsg* p_sMsg) {

    if (g_fpLog == NULL)
        return 1;

    fprintf(g_fpLog, "%s%s %s\n", p_sMsg->m_sFormat, p_sMsg->m_sId, p_sMsg->m_sMsg);
    fflush(g_fpLog);

    return 0;
}

int _file_handler_open() {
    g_fpLog = fopen(g_sFileWithPath, "a"); // TODO make variable

    if (g_fpLog == NULL) {
        // Failed to open the log file
        // FIXME include full path to file
        fprintf(stderr, "Failed to open the log file at: %s\n", g_sFileWithPath);
        return 1;
    }
    return 0;
}

int _file_handler_isOpen() {
    if (g_fpLog != NULL) {
        return 1;
    }
    else {
        return 0;
    }
}
// END PRIVATE FUNCTION DEFINITIONS


int create_file_handler(log_handler *p_pHandler, char* p_sLogLocation, char* p_sLogName) {

    // FIXME can only support one file at a time right now
    if (g_fpLog != NULL) {
        fprintf(stderr, "Can't create file handler; there's already an active file handler.\n");
        return 1;
    }

    // Check if p_sLogLocation is a directory
    if (p_sLogLocation != NULL) {
        if (lgu_is_dir(p_sLogLocation) != 0) {
            // directory does not exist; try to create it
            int t_nDirRtn = mkdir(p_sLogLocation, 0755);
            if (t_nDirRtn != 0) {
                fprintf(stderr, "Failed to create log directory\n");
                return 1;
            }
        }
        else {
            // directory exists; check if we have write perms
            if (lgu_can_write(p_sLogLocation) != 0) {
                fprintf(stderr, "ERROR! Do not have permission to write logs to '%s'\n", p_sLogLocation);
                return 1;
            }
        }
    }
    else {
        // log will be placed in directory the program is running from
        p_sLogLocation = (char*) "./";
    }

    char* t_sLogName;
    if (p_sLogName != NULL)
        t_sLogName = p_sLogName;
    else
        t_sLogName = (char*) "log.log";

    // FIXME need to check the sizes of p_sLogLocation and p_sLogName

    // try to open the log file
    {
        size_t sn_rtn = snprintf(
            g_sFileWithPath,
            sizeof(char) * MAX_LEN_FILE_W_PATH,
            "%s/%s",
            p_sLogLocation,
            t_sLogName
        );
        if (sn_rtn >= sizeof(char) * MAX_LEN_FILE_W_PATH) {
            // the file with path was too long
            fprintf(stderr, "Cannot create file handler because file with path is too long.\n");
            return 1;
        }
    }

    log_handler t_structHandler = {
        &_file_handler_write,
        &_file_handler_close,
        &_file_handler_open,
        &_file_handler_isOpen,
        true,
        true
    };
    
    // TODO can we check perms on the file without opening it? should we open and close
    // quickly to test?

    memcpy(p_pHandler, &t_structHandler, sizeof(log_handler));

    return 0;
}

