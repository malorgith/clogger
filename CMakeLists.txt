cmake_minimum_required(VERSION 3.10)

# set the project name
project(clogger_prj VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CLOGGER_BUILD_VAR_RELEASE "Release")

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)
include(GNUInstallDirs)

set(logger_src_files
    src/logger_util.c
    src/logger.c
    src/logger_buffer.c
    src/logger_formatter.c
    src/logger_handler.c
    src/logger_id.c
    src/logger_levels.c
    src/logger_msg.c
    src/handlers/console_handler.c
    src/handlers/file_handler.c
)

set(clogger_default_target_name "clogger")
set(clogger_debug_target_name "${clogger_default_target_name}d")
set(clogger_static_target_name "${clogger_default_target_name}_static")
set(clogger_debug_static_target_name "${clogger_debug_target_name}_static")

# init these to default values
set(clogger_shared_target "${clogger_default_target_name}")
set(clogger_static_target "${clogger_static_target_name}")

set(default_compile_opts -Wall -Wextra)

option(CLOGGER_BUILD_SHARED "build the shared library; default" ON)
option(CLOGGER_BUILD_STATIC "build a static version of the library" OFF)
option(CLOGGER_ENABLE_GRAYLOG "build support for a Graylog handler" ON)
option(CLOGGER_ENABLE_VERBOSE_WARNING "warning messages will be printed to stderr when functions fail" OFF)
option(CLOGGER_BUILD_EXAMPLES "enables options for building example/debugging programs" OFF)
option(CLOGGER_NO_DEBUG_WARNING "don't output warning messages when running a non-release build of the library" OFF)
option(CLOGGER_NO_SLEEP "don't sleep while waiting for messages; increases CPU usage" OFF)

# TODO the source files can be set below, but those aren't inherited by other
# libs that link this
add_library(base_target INTERFACE)
set_target_properties(base_target PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/src/clogger.h"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/${CMAKE_INSTALL_LIBDIR}"
)
target_compile_options(base_target INTERFACE ${default_compile_opts})

# TODO archive destination and library destination aren't picked up automatically
install(TARGETS base_target
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(
	${CMAKE_BUILD_TYPE} STREQUAL ${CLOGGER_BUILD_VAR_RELEASE} OR
	${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo"
)
    # treat warnings as errors for release
    target_compile_options(base_target INTERFACE -Werror)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    # update the target names to use the debug names
    set(clogger_shared_target "${clogger_debug_target_name}")
    set(clogger_static_target "${clogger_debug_static_target_name}")
endif()

# need to put this after most definitions
add_subdirectory(src/examples)

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

if(CLOGGER_ENABLE_GRAYLOG)
    set(logger_src_files ${logger_src_files} src/handlers/graylog_handler.c)
    target_compile_options(base_target INTERFACE -DCLOGGER_GRAYLOG)

    set(CLOGGER_INCLUDE_FILES ${CLOGGER_INCLUDE_FILES}
        arpa/inet.h
        netdb.h
        sys/socket.h
    )

#   check_type_size(addrinfo SIZE_ADDRINFO)

    set(CLOGGER_SYMBOL_CHECKS ${CLOGGER_SYMBOL_CHECKS}
        getaddrinfo
        freeaddrinfo
        gai_strerror
        memset
        gethostname
        close
    )

endif()

if(CLOGGER_ENABLE_VERBOSE_WARNING)
    target_compile_options(base_target INTERFACE -DCLOGGER_VERBOSE_WARNING)
endif()

if(CLOGGER_NO_DEBUG_WARNING)
    target_compile_options(base_target INTERFACE -DCLOGGER_REMOVE_WARNING)
endif()

if(CLOGGER_NO_SLEEP)
    target_compile_options(base_target INTERFACE -DCLOGGER_NO_SLEEP)
endif()

if(CLOGGER_BUILD_STATIC)
    add_library(${clogger_static_target} STATIC ${logger_src_files})
    target_link_libraries(${clogger_static_target} PUBLIC base_target)
    # FIXME we don't want to install debug versions
    install(TARGETS ${clogger_static_target}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(CLOGGER_BUILD_SHARED)
    add_library(${clogger_shared_target} SHARED ${logger_src_files})
    target_link_libraries(${clogger_shared_target} PUBLIC base_target)
    # FIXME we don't want to install debug version
    install(TARGETS ${clogger_shared_target}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(NOT CLOGGER_BUILD_SHARED AND NOT CLOGGER_BUILD_STATIC)
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

