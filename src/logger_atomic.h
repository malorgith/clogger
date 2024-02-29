#ifndef MALORGITH_CLOGGER_ATOMIC_H_
#define MALORGITH_CLOGGER_ATOMIC_H_

#ifdef __cplusplus
#pragma once
#include <atomic>
typedef ::std::atomic<bool> MalorgithAtomicBool;
typedef ::std::atomic<int> MalorgithAtomicInt;
#define MALORGITH_CLOGGER_ATOMIC_BOOL ::std::atomic<bool>
#define MALORGITH_CLOGGER_ATOMIC_INT ::std::atomic<int>
#else
#include <stdatomic.h>
typedef atomic_bool MalorgithAtomicBool;
typedef atomic_int MalorgithAtomicInt;
#define MALORGITH_CLOGGER_ATOMIC_BOOL atomic_bool
#define MALORGITH_CLOGGER_ATOMIC_INT atomic_int
#endif  // __cplusplus

#endif  // MALORGITH_CLOGGER_ATOMIC_H_
