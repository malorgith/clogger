
#ifndef LOGGER_UTIL_H_INCLUDED
#define LOGGER_UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>

int lgu_add_to_time(struct timespec *p_pTspec, int ms_to_add, int min_ms, int max_ms);

int lgu_can_write(const char* p_sFile);
int lgu_get_size(const char* p_sFilename);
int lgu_is_dir(const char* p_sDirectory);
bool lgu_is_file(const char* p_sFilename);

int lgu_wsnprintf(char* p_sDest, int p_nMaxSize, const char* p_sWarnMsg, const char* p_sFormat, ...);

void lgu_warn_msg(const char* p_sMsg);
void lgu_warn_msg_int(const char* p_sMsgFormat, const int p_nPut);
void lgu_warn_msg_str(const char* p_sMsgFormat, const char *p_sPut);

#ifdef __cplusplus
}
#endif

#endif
