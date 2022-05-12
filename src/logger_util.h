
#ifndef LOGGER_UTIL_H_INCLUDED
#define LOGGER_UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int lgu_can_write(char* p_sFile);
int lgu_get_size(char* p_sFilename);
int lgu_is_dir(char* p_sDirectory);
bool lgu_is_file(char* p_sFilename);

void lgu_warn_msg(const char* p_sMsg);
void lgu_warn_msg_int(const char* p_sMsgFormat, const int p_nPut);

#ifdef __cplusplus
}
#endif

#endif
