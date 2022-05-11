
#include "disk_util.h"

#include <sys/stat.h>
#include <unistd.h>


int disk_util_can_write(char* file) {

    int access_rtn = access(file, W_OK);
    return access_rtn;
}

int disk_util_get_size(char* p_sFilename) {

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

int disk_util_is_dir(char* directory) {

    struct stat sb;

    if (stat(directory, &sb) == -1)
        return -1;
    else
        if (S_ISDIR(sb.st_mode))
            return 0;
        else
            return 1;

}

bool disk_util_is_file(char* p_sFilename) {

	struct stat t_statBuf;
	if (stat(p_sFilename, &t_statBuf) == -1)
	    return false;
	else
	    if (S_ISREG(t_statBuf.st_mode))
	        return true;               
	    else
	        return false;

}
