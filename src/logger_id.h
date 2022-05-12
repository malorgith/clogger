
#ifndef LOGGER_ID_H_INCLUDED
#define LOGGER_ID_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "clogger.h"

#define LOGGER_ID_MAX_IDS   20

int lgi_init();
int lgi_free();

int lgi_add_id(char* p_sID);
int lgi_get_id(logger_id id_ref, char* dest);
int lgi_remove_id(logger_id id_ref);

logger_id lgi_add_id(char* p_sId);

int lgi_get_longest_len();

#ifdef __cplusplus
}
#endif

#endif
