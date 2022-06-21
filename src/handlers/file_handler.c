
#include "file_handler.h"

#include "../logger_util.h"

#include <stdlib.h>
#include <string.h> // memcpy()
#include <sys/stat.h>   // mkdir()

#define MAX_LEN_FILE_W_PATH 80

// global variables
typedef struct {
    FILE* m_pLog;
    char m_sFileWithPath[MAX_LEN_FILE_W_PATH];
} filehandler_data;

static const size_t g_sizeData = { sizeof(filehandler_data) };

// private function declarations
static int _file_handler_close(log_handler *p_pHandler);
static int _file_handler_isOpen(const log_handler *p_pHandler);
static int _file_handler_open(log_handler *p_pHandler);
static int _file_handler_write(log_handler* p_pHandler, const t_loggermsg* p_sMsg);

// private function definitions
int _file_handler_close(log_handler *p_pHandler) {
    filehandler_data* t_pData = (filehandler_data*)p_pHandler->m_pHandlerData;

    if (t_pData->m_pLog == NULL) {
        return 1;
    }
    if (fclose(t_pData->m_pLog)) {
        // file didn't close successfully
        lgu_warn_msg("failed to close file handler");
    }
    t_pData->m_pLog = NULL;
    free(p_pHandler->m_pHandlerData);
    p_pHandler->m_pHandlerData = NULL;

    return 0;
}

int _file_handler_isOpen(const log_handler *p_pHandler) {
    if (((filehandler_data*)p_pHandler->m_pHandlerData)->m_pLog != NULL) return 1;
    else return 0;
}

int _file_handler_open(log_handler *p_pHandler) {

    if (lgh_checks(p_pHandler, g_sizeData, "file handler"))
        return 1;

    filehandler_data* t_pData = (filehandler_data*)p_pHandler->m_pHandlerData;
    if (t_pData->m_pLog != NULL) {
        lgu_warn_msg("can't open log because it's already open");
        return 1;
    }

    t_pData->m_pLog = fopen(t_pData->m_sFileWithPath, "a"); // TODO make variable

    if (t_pData->m_pLog == NULL) {
        // Failed to open the log file
        // TODO include full path to file
        lgu_warn_msg("failed to open log file");
        return 1;
    }
    return 0;
}

int _file_handler_write(log_handler *p_pHandler, const t_loggermsg* p_sMsg) {

    if (lgh_checks(p_pHandler, g_sizeData, "file handler"))
        return 1;
    else if (p_sMsg == NULL) {
        lgu_warn_msg("file handler given NULL message to log");
        return 1;
    }

    filehandler_data *t_pData = (filehandler_data*)p_pHandler->m_pHandlerData;

    fprintf(t_pData->m_pLog, "%s%s %s\n", p_sMsg->m_sFormat, p_sMsg->m_sId, p_sMsg->m_sMsg);
    fflush(t_pData->m_pLog);

    return 0;
}

// public function definitions
int create_file_handler(log_handler *p_pHandler, const char* p_sLogLocation, const char* p_sLogName) {

    // for now we will allow the location to be NULL to indicate current directory

    if (p_pHandler == NULL) {
        lgu_warn_msg("cannot create file handler in NULL ptr");
        return 1;
    }
    else if (p_sLogName == NULL) {
        lgu_warn_msg("the log name must be supplied");
        return 1;
    }

    // TODO we should do checks on the name given; since we're expecting the name and
    // path individually, we need to make sure the name doesn't contain path separators

    /*
     * It's unfortunate that we have to use snprintf to store both the
     * name and directory before just using it again to store both of
     * them combined, but there doesn't seem to be a better way to do
     * it while allowing the char* params to come in as const.
     */

    static const int m_nMaxPathLen = 50;
    static const int m_nMaxLogNameLen = 30;

    static const char* m_sDefaultLogDir = "./";

    char t_sLogDir[m_nMaxPathLen];
    char t_sLogName[m_nMaxLogNameLen];

    // Check if p_sLogLocation is a directory
    if (p_sLogLocation != NULL) {
        if (lgu_is_dir(p_sLogLocation)) {
            // directory does not exist; try to create it
            int t_nDirRtn = mkdir(p_sLogLocation, 0755);
            if (t_nDirRtn != 0) {
                lgu_warn_msg("failed to create log directory");
                return 1;
            }
        }
        else {
            // directory exists; check if we have write perms
            if (lgu_can_write(p_sLogLocation)) {
                fprintf(stderr, "ERROR! Do not have permission to write logs to '%s'\n", p_sLogLocation);
                return 1;
            }
        }
        static const char* t_sErrMsg = "failed to store the log path given";
        if (lgu_wsnprintf(t_sLogDir, m_nMaxPathLen, t_sErrMsg, "%s", p_sLogLocation)) return 1;
    }
    else {
        // put logs in default log dir
        static const char* t_sErrMsg = "failed to store the default log directory in file handler";
        if (lgu_wsnprintf(t_sLogDir, m_nMaxPathLen, t_sErrMsg, "%s", m_sDefaultLogDir)) return 1;
    }

    // store the log file name
    {
        static const char* t_sErrMsg = "failed to store the log name given";
        if (lgu_wsnprintf(t_sLogName, m_nMaxLogNameLen, t_sErrMsg, "%s", p_sLogName)) return 1;
    }

    // allocate space to store data specific to this type of handler
    filehandler_data* t_pData = (filehandler_data*)malloc(sizeof(filehandler_data));
    if (t_pData == NULL) {
        // failed to allocate space for the data
        return 1;
    }
    t_pData->m_pLog = NULL;

    // build the full path to the file
    {
        static const char* t_sErrMsg = "failed to combine and store log path and file name";
        static const int t_nSize = sizeof(char) * MAX_LEN_FILE_W_PATH;
        if (lgu_wsnprintf(
            t_pData->m_sFileWithPath,
            t_nSize,
            t_sErrMsg,
            "%s/%s",
            t_sLogDir,
            t_sLogName
        )) {
            return 1;
        }
    }

    log_handler t_structHandler = {
        &_file_handler_write,
        &_file_handler_close,
        &_file_handler_open,
        &_file_handler_isOpen,
        true,
        true,
        t_pData
    };
    
    // TODO can we check perms on the file without opening it? should we open and close
    // quickly to test?

    memcpy(p_pHandler, &t_structHandler, sizeof(log_handler));

    return 0;
}

