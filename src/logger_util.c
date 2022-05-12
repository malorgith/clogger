
#include "logger_util.h"

#include <stdio.h> //fprintf(), snprintf()
#include <sys/stat.h>
#include <unistd.h>


// public functions
int lgu_can_write(char* p_sFileWithPath) {

    int t_nRtn = access(p_sFileWithPath, W_OK);
    return t_nRtn;
}

int lgu_get_size(char* p_sFilename) {

    // get the size (in bytes) of the file

    struct stat t_statStruct;
    if (stat(p_sFilename, &t_statStruct) == -1) {
        return -1;
    }
    else {
        // get the size from the stat struct
        return t_statStruct.st_size;
    }

    // shouldn't be possible to get here
    return -1;
}

int lgu_is_dir(char* p_sDirectory) {

    struct stat sb;

    if (stat(p_sDirectory, &sb) == -1)
        return -1;
    else
        if (S_ISDIR(sb.st_mode))
            return 0;
        else
            return 1;

}

bool lgu_is_file(char* p_sFilename) {

	struct stat t_statBuf;
	if (stat(p_sFilename, &t_statBuf) == -1)
	    return false;
	else
	    if (S_ISREG(t_statBuf.st_mode))
	        return true;
	    else
	        return false;

}

#ifdef CLOGGER_VERBOSE_WARNING
/*
 * Print the error message to stderr
 */
void lgu_warn_msg(const char* p_sMsg) {
    fprintf(stderr, "clogger error: %s\n", p_sMsg);
    fflush(stderr);
}

void lgu_warn_msg_int(const char* p_sMsgFormat, const int p_nPut) {
    static const int t_nMaxMsgSize = 300;
    char t_sMsg[t_nMaxMsgSize];

    int t_nFormatRtn = snprintf(t_sMsg, t_nMaxMsgSize, p_sMsgFormat, p_nPut);
    if ((t_nFormatRtn == 0) || (t_nFormatRtn >= t_nMaxMsgSize)) {
        // failed to format the message
        lgu_warn_msg("Encountered an error, but failed to format output.");
    }
    else {
        lgu_warn_msg(t_sMsg);
    }
}
#else
/*
 * Ignore the message
 */
void lgu_warn_msg(__attribute__((unused))const char* p_sMsg) {}
void lgu_warn_msg_int(__attribute__((unused))const char* p_sMsgFormat, __attribute__((unused))const int p_nPut) {}
#endif

