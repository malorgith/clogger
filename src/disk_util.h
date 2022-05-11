
#ifndef DISK_UTIL_H_INCLUDED
#define DISK_UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int disk_util_can_write(char* file);
int disk_util_get_size(char* p_sFilename);
int disk_util_is_dir(char* directory);
bool disk_util_is_file(char* p_sFilename);

#ifdef __cplusplus
}
#endif

#endif
