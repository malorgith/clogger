
#include "handlers/file_handler.h"

#include <stdlib.h>
#include <string.h>  // memcpy()
#include <sys/stat.h>   // mkdir()

#include "logger_defines.h"
#include "logger_util.h"

#define MAX_LEN_FILE_W_PATH 80

__MALORGITH_NAMESPACE_OPEN

// global variables
typedef struct {
    FILE* log_;
    char file_path_str_[MAX_LEN_FILE_W_PATH];
} filehandler_data;

static const size_t kDataSize = { sizeof(filehandler_data) };

// private function declarations
static int _file_handler_close(log_handler *handler_ptr);
static int _file_handler_isOpen(const log_handler *handler_ptr);
static int _file_handler_open(log_handler *handler_ptr);
static int _file_handler_write(log_handler* handler_ptr, const t_loggermsg* msg);

// private function definitions
int _file_handler_close(log_handler *handler_ptr) {
    filehandler_data* data_ptr = (filehandler_data*)handler_ptr->handler_data_;

    if (data_ptr->log_ == NULL) {
        return 1;
    }
    if (fclose(data_ptr->log_)) {
        // file didn't close successfully
        lgu_warn_msg("failed to close file handler");
    }
    data_ptr->log_ = NULL;
    free(handler_ptr->handler_data_);
    handler_ptr->handler_data_ = NULL;

    return 0;
}

int _file_handler_isOpen(const log_handler *handler_ptr) {
    if (((filehandler_data*)handler_ptr->handler_data_)->log_ != NULL) return 1;
    else return 0;
}

int _file_handler_open(log_handler *handler_ptr) {

    if (lgh_checks(handler_ptr, kDataSize, "file handler"))
        return 1;

    filehandler_data* data_ptr = (filehandler_data*)handler_ptr->handler_data_;
    if (data_ptr->log_ != NULL) {
        lgu_warn_msg("can't open log because it's already open");
        return 1;
    }

    data_ptr->log_ = fopen(data_ptr->file_path_str_, "a"); // TODO make variable

    if (data_ptr->log_ == NULL) {
        // Failed to open the log file
        // TODO include full path to file
        lgu_warn_msg("failed to open log file");
        return 1;
    }
    return 0;
}

int _file_handler_write(log_handler *handler_ptr, const t_loggermsg* msg) {

    if (lgh_checks(handler_ptr, kDataSize, "file handler"))
        return 1;
    else if (msg == NULL) {
        lgu_warn_msg("file handler given NULL message to log");
        return 1;
    }

    filehandler_data *data_ptr = (filehandler_data*)handler_ptr->handler_data_;

    fprintf(data_ptr->log_, "%s%s %s\n", msg->format_, msg->id_, msg->msg_);
    fflush(data_ptr->log_);

    return 0;
}

// public function definitions
int create_file_handler(log_handler *handler_ptr, char const* log_location_str, char const* log_name_str) {

    // for now we will allow the location to be NULL to indicate current directory

    if (handler_ptr == NULL) {
        lgu_warn_msg("cannot create file handler in NULL ptr");
        return 1;
    }
    else if (log_name_str == NULL) {
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

    static const int kMaxPathLen = 50;
    static const int kMaxLogNameLen = 30;

    static char const* kDefaultLogDir = "./";

    char log_dir_str[kMaxPathLen];
    char log_name_char[kMaxLogNameLen];

    // Check if log_location_str is a directory
    if (log_location_str != NULL) {
        if (lgu_is_dir(log_location_str)) {
            // directory does not exist; try to create it
            int mkdir_rtn = mkdir(log_location_str, 0755);
            if (mkdir_rtn != 0) {
                lgu_warn_msg("failed to create log directory");
                return 1;
            }
        }
        else {
            // directory exists; check if we have write perms
            if (lgu_can_write(log_location_str)) {
                fprintf(stderr, "ERROR! Do not have permission to write logs to '%s'\n", log_location_str);
                return 1;
            }
        }
        static char const* kErrMsg = "failed to store the log path given";
        if (lgu_wsnprintf(log_dir_str, kMaxPathLen, kErrMsg, "%s", log_location_str)) return 1;
    }
    else {
        // put logs in default log dir
        static char const* kErrMsg = "failed to store the default log directory in file handler";
        if (lgu_wsnprintf(log_dir_str, kMaxPathLen, kErrMsg, "%s", kDefaultLogDir)) return 1;
    }

    // store the log file name
    {
        static char const* kErrMsg = "failed to store the log name given";
        if (lgu_wsnprintf(log_name_char, kMaxLogNameLen, kErrMsg, "%s", log_name_str)) return 1;
    }

    // allocate space to store data specific to this type of handler
    filehandler_data* data_ptr = (filehandler_data*)malloc(sizeof(filehandler_data));
    if (data_ptr == NULL) {
        // failed to allocate space for the data
        return 1;
    }
    data_ptr->log_ = NULL;

    // build the full path to the file
    {
        static char const* kErrMsg = "failed to combine and store log path and file name";
        static const int file_path_max_size = sizeof(char) * MAX_LEN_FILE_W_PATH;
        if (lgu_wsnprintf(
            data_ptr->file_path_str_,
            file_path_max_size,
            kErrMsg,
            "%s/%s",
            log_dir_str,
            log_name_char
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
        data_ptr
    };
    // TODO can we check perms on the file without opening it? should we open and close
    // quickly to test?

    memcpy(handler_ptr, &t_structHandler, sizeof(log_handler));

    return 0;
}

__MALORGITH_NAMESPACE_CLOSE
