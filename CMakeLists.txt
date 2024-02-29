cmake_minimum_required(VERSION 3.10)

# set the project name
project(clogger_prj
    VERSION 0.1.0
    DESCRIPTION "A multi-threaded logger written in C"
    HOMEPAGE_URL "https://github.com/malorgith/clogger"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CLOGGER_BUILD_VAR_RELEASE "Release")

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)
include(GNUInstallDirs)

set(logger_src_files
    ${PROJECT_SOURCE_DIR}/src/logger_defines.c
    ${PROJECT_SOURCE_DIR}/src/logger_util.c
    ${PROJECT_SOURCE_DIR}/src/logger_buffer.c
    ${PROJECT_SOURCE_DIR}/src/logger_formatter.c
    ${PROJECT_SOURCE_DIR}/src/logger_handler.c
    ${PROJECT_SOURCE_DIR}/src/logger_id.c
    ${PROJECT_SOURCE_DIR}/src/logger_levels.c
    ${PROJECT_SOURCE_DIR}/src/logger_msg.c
    ${PROJECT_SOURCE_DIR}/src/handlers/console_handler.c
    ${PROJECT_SOURCE_DIR}/src/handlers/file_handler.c
    ${PROJECT_SOURCE_DIR}/src/logger.c
)
set(clogger_public_h_location "${CMAKE_BINARY_DIR}/generated/clogger.h")

set(clogger_shared_output_name "clogger$<$<CONFIG:Debug>:d>")
set(clogger_static_output_name "${clogger_shared_output_name}_static")

option(BUILD_SHARED_LIBS "build the shared library; default" ON)
option(BUILD_STATIC_LIBS "build a static version of the library" OFF)
option(CLOGGER_ENABLE_VERBOSE_WARNING "warning messages will be printed to stderr when functions fail" OFF)
option(CLOGGER_BUILD_EXAMPLES "enables options for building example/debugging programs" OFF)
option(CLOGGER_NO_DEBUG_WARNING "don't output warning messages when running a non-release build of the library" OFF)
option(CLOGGER_BUILD_CC "build the program using c++" OFF)

set(CLOGGER_MAX_MESSAGE_SIZE "200" CACHE STRING "The maximum message size")
set(CLOGGER_ID_MAX_LEN "40" CACHE STRING "The maximum ID length")
set(CLOGGER_MAX_NUM_IDS "20" CACHE STRING "The maximum number of IDs that can be stored at the same time")
set(CLOGGER_MAX_NUM_HANDLERS "5" CACHE STRING "The maximum number of handlers that can be stored at the same time")
set(CLOGGER_BUFFER_SIZE "50" CACHE STRING "The number of messages that can be stored on the buffer before they're written")

add_library(base_target INTERFACE)
target_compile_options(base_target INTERFACE
    -Wall
    -Wextra
    -DCLOGGER_MAX_MESSAGE_SIZE=${CLOGGER_MAX_MESSAGE_SIZE}
    -DCLOGGER_ID_MAX_LEN=${CLOGGER_ID_MAX_LEN}
    -DCLOGGER_MAX_NUM_IDS=${CLOGGER_MAX_NUM_IDS}
    -DCLOGGER_MAX_NUM_HANDLERS=${CLOGGER_MAX_NUM_HANDLERS}
    -DCLOGGER_BUFFER_SIZE=${CLOGGER_BUFFER_SIZE}
)
target_include_directories(base_target INTERFACE
    "src"
    "${CMAKE_BINARY_DIR}/generated/"
)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CLOGGER_BUILD_VAR_RELEASE})
endif()

if(
    ${CMAKE_BUILD_TYPE} STREQUAL ${CLOGGER_BUILD_VAR_RELEASE} OR
    ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo"
)
    # treat warnings as errors for release
    target_compile_options(base_target INTERFACE -Werror)
endif()

set(CLOGGER_INCLUDE_FILES
    semaphore.h
    stdbool.h
    stdint.h
    stdio.h
    stdlib.h
    string.h
    sys/stat.h
    time.h
    unistd.h
    errno.h
    stdatomic.h
    stdarg.h
    pthread.h
)

set(CLOGGER_SYMBOL_CHECKS
    malloc
    realloc
    free
    strlen
    strcmp
    time
    strftime
    localtime_r
    printf
    fprintf
    mkdir
    fflush
    fopen
    snprintf
    fclose
    access
    stat
    sem_init
    sem_post
    sem_destroy
    sem_wait
    sem_trywait
    sem_timedwait
    pthread_exit
    pthread_create
    pthread_join
    va_start
    va_copy
    va_arg
    va_end
)

# TODO can we check for sem_t? can't find a way
#check_type_size(tm SIZE_TM)
#check_type_size(bool SIZE_BOOL)
#check_type_size("atomic_int" SIZE_ATOMICINT)

if(CLOGGER_BUILD_CC)
    set(clogger_cc_public_h_location "${CMAKE_BINARY_DIR}/generated/clogger.hpp")
endif()

add_subdirectory(cmake) # must be before others to get graylog info
include(cmake/AddLibraryTarget.cmake) # get function def
if(CLOGGER_BUILD_CC)
    add_subdirectory(src/cc)
endif()
add_subdirectory(src/test)
add_subdirectory(src/examples)

if(CLOGGER_ADD_MACROS)
   target_compile_options(base_target INTERFACE
       -DCLOGGER_BUILD_MACROS=1
   )
endif()

if(CLOGGER_ENABLE_VERBOSE_WARNING)
    target_compile_options(base_target INTERFACE -DCLOGGER_VERBOSE_WARNING)
endif()

if(CLOGGER_NO_DEBUG_WARNING OR (${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo"))
    target_compile_options(base_target INTERFACE -DCLOGGER_REMOVE_WARNING)
endif()

if(BUILD_STATIC_LIBS)
    CloggerAddLibraryTarget(
        clogger_static
        STATIC
        "${logger_src_files}"
        "${clogger_public_h_location}"
        ${clogger_static_output_name}
    )
endif()

if(BUILD_SHARED_LIBS)
    CloggerAddLibraryTarget(
        clogger_shared
        SHARED
        "${logger_src_files}"
        "${clogger_public_h_location}"
        ${clogger_shared_output_name}
    )
endif()

if(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "Must specify a library to build")
endif()

foreach(HEADER_TEST ${CLOGGER_INCLUDE_FILES})
    check_include_file(${HEADER_TEST} CLOGGER_${HEADER_TEST}_FOUND)
    if(NOT CLOGGER_${HEADER_TEST}_FOUND)
        message(FATAL_ERROR "Missing required header '${HEADER_TEST}'")
    endif()
endforeach()

foreach(SYMBOL_TEST ${CLOGGER_SYMBOL_CHECKS})
    check_symbol_exists(${SYMBOL_TEST} "${CLOGGER_INCLUDE_FILES}" CLOGGER_HAVE_${SYMBOL_TEST})
    if(NOT CLOGGER_HAVE_${SYMBOL_TEST})
        message(FATAL_ERROR "Missing required symbol '${SYMBOL_TEST}'")
    endif()
endforeach()
