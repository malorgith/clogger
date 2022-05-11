
#ifndef LOGGER_ID_H_INCLUDED
#define LOGGER_ID_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "clogger.h"

#define LOGGER_ID_MAX_IDS   20

int logger_id_init();
int logger_id_free();

int logger_id_add_id(char* p_sID);
int logger_id_get_id(logger_id id_ref, char* dest);
int logger_id_remove_id(logger_id id_ref);

logger_id logger_id_add_id(char* p_sId);

int logger_id_get_longest_len();

#ifdef __cplusplus
}
#endif

#endif
