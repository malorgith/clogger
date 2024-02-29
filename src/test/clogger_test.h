#ifndef MALORGITH_CLOGGER_TEST_CLOGGER_TEST_H_
#define MALORGITH_CLOGGER_TEST_CLOGGER_TEST_H_

#ifdef __cplusplus
#pragma once
#endif  // __cplusplus

#include <CUnit/CUnit.h>

int init_buffer_suite(CU_pSuite suite);
int init_format_suite(CU_pSuite suite);
int init_handler_suite(CU_pSuite suite);
int init_id_suite(CU_pSuite suite);
int init_level_suite(CU_pSuite suite);
int init_logger_suite(CU_pSuite suite);
int init_msg_suite(CU_pSuite suite);

#endif  // MALORGITH_CLOGGER_TEST_CLOGGER_TEST_H_
