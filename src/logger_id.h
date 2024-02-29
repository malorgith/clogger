#ifndef MALORGITH_CLOGGER_ID_H_
#define MALORGITH_CLOGGER_ID_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

//#include "clogger.h"
#include "logger_defines.h"
#include __MALORGITH_CLOGGER_INCLUDE

__MALORGITH_NAMESPACE_OPEN

/*
 * @brief Initialize log ID globals.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_init();

/*
 * @brief Free log ID globals.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_free();

/*
 * @brief Create a new log ID.
 *
 * @returns a logid_t reference to the new ID on success, CLOGGER_MAX_NUM_IDS
 * on failure
 */
logid_t lgi_add_id(
    /*! the value of the ID */
    char const* str_id
);

/*
 * @brief Create a new ID with the specified handlers.
 *
 * @returns a logid_t reference to the new ID on success, CLOGGER_MAX_NUM_IDS
 * on failure
 */
logid_t lgi_add_id_w_handler(
    /*! the name of the ID to create */
    char const* str_id,
    /*! the handler(s) to assign to the ID */
    loghandler_t p_refHandlers
);

/*
 * @brief Add a handler to the specified logid_t.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_add_handler(
    /*! the ID to add the handler(s) to */
    logid_t,
    /*! the handler(s) to add */
    loghandler_t
);

/*
 * @brief Increment the message count for the specified logid_t.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_add_message(
    /*! the ID to add a message to */
    logid_t
);

/*
 * @brief Store the value of the specified logid_t in the destination.
 *
 * @returns a loghandler_t that represents the handlers that should be written
 * to on success, kCloggerHandlerErr on error
 */
loghandler_t lgi_get_id(
    /*! the ID to fetch the value of */
    logid_t id_index,
    /*! the location to store the ID value */
    char* dest
);

/*
 * @brief Remove a handler from the specified logid_t.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_remove_handler(
    /*! the ID to remove the handler(s) from */
    logid_t,
    /*! the handler(s) to remove */
    loghandler_t
);

/*
 * @brief Remove the specified logid_t.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_remove_id(
    /*! the ID to remove */
    logid_t id_index
);

/*
 * @brief Decrease the message count for the specified logid_t.
 *
 * @returns 0 on success, non-zero on failure
 */
int lgi_remove_message(
    /*! the ID to remove the message from */
    logid_t
);

__MALORGITH_NAMESPACE_CLOSE

#endif  // MALORGITH_CLOGGER_ID_H_
