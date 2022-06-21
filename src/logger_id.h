
#ifndef LOGGER_ID_H_INCLUDED
#define LOGGER_ID_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "clogger.h"

/*
 * Initializes global variables.
 *
 * Returns 0 on success, non-zero on failure.
 */
int lgi_init();

/*
 * Frees all id data.
 *
 * Returns 0 on success, non-zero on failure.
 */
int lgi_free();

/*
 * Creates a new id.
 *
 * Returns a logger_id reference to the new id on success,
 * CLOGGER_MAX_NUM_IDS on failure.
 */
logger_id lgi_add_id(const char* p_sId);

/*
 * Creates a new id with the specified handlers.
 *
 * Returns a logger_id reference to the new id on success,
 * CLOGGER_MAX_NUM_IDS on failure.
 */
logger_id lgi_add_id_w_handler(const char* p_sId, t_handlerref p_refHandlers);

/*
 * Add a new handler to the specified logger_id
 */
int lgi_add_handler(logger_id, t_handlerref);

/*
 * Increments the message count for the id at the specified reference.
 *
 * Returns 0 on success, non-zero on failure.
 */
int lgi_add_message(logger_id);

/*
 * Stores the value of the log id at the specified reference in dest.
 *
 * Returns a t_handlerref that represents the handlers that should
 * be written to on success, and CLOGGER_HANDLER_ERR on error.
 */
t_handlerref lgi_get_id(logger_id p_refId, char* dest);

/*
 * Removes a handler from the specified logger_id
 */
int lgi_remove_handler(logger_id, t_handlerref);

/*
 * Removes the log id at the specified reference.
 *
 * Returns 0 on success, non-zero on failure.
 */
int lgi_remove_id(logger_id p_refId);

/*
 * Decreases the message count for the id at the specified reference.
 *
 * Returns 0 on success, non-zero on failure.
 */
int lgi_remove_message(logger_id);

#ifdef __cplusplus
}
#endif

#endif
