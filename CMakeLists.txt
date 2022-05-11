cmake_minimum_required(VERSION 3.10)

# set the project name
project(clogger_prj VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CLOGGER_BUILD_VAR_RELEASE "Release")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CLOGGER_BUILD_VAR_RELEASE} CACHE STRING "Debug, Release" FORCE)
endif()

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)
include(GNUInstallDirs)

set(logger_src_files
    src/disk_util.c
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
set(clogger_debug_target_name "${clogger_default_target_name}_debug")
set(clogger_static_target_name "${clogger_default_target_name}_static")
set(clogger_testlib_target_name "${clogger_default_target_name}_test")
set(clogger_testbin_target_name "${clogger_default_target_name}_test_bin")

set(default_compile_opts -Wall -Wextra)

option(CLOGGER_BUILD_SHARED "build the shared library; default" ON)
option(CLOGGER_BUILD_STATIC "build a static version of the library" OFF)
option(CLOGGER_BUILD_TEST "build an executable to test the logger" OFF)
option(CLOGGER_ENABLE_GRAYLOG "build support for a Graylog handler" ON)
option(CLOGGER_STRESS_TEST "add a stress test to the test binary" OFF)

# XXX the source files can be set below, but those aren't inherited by other
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

if(CMAKE_BUILD_TYPE STREQUAL ${CLOGGER_BUILD_VAR_RELEASE})
    # strip the final release binary
    target_link_options(base_target INTERFACE -s)
    # treat warnings as errors for release
    target_compile_options(base_target INTERFACE -Werror)
endif()

if(CLOGGER_STRESS_TEST)
    # the test binary has to be built for the stress test
    set(CLOGGER_BUILD_TEST ON)
endif()

if(CLOGGER_BUILD_TEST)
    # create the test lib
    add_library(${clogger_testlib_target_name} SHARED ${logger_src_files})
    # link with the base interface to inherit properties
    target_link_libraries(${clogger_testlib_target_name} PUBLIC base_target)

    # update the compile options to build the testing functions
    target_compile_options(${clogger_testlib_target_name} PRIVATE -DTESTING_FUNCTIONS_ENABLED)

    set(CLOGGER_SYMBOL_CHECKS ${CLOGGER_SYMBOL_CHECKS}
        sleep
    )

    # create the test binary
    add_executable(${clogger_testbin_target_name} src/logger_test.c)
    # set the output location for the test binary
    set_target_properties(${clogger_testbin_target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
    )

    target_link_libraries(${clogger_testbin_target_name}
        ${clogger_testlib_target_name}
    )
    target_compile_options(${clogger_testbin_target_name} PRIVATE -DTESTING_FUNCTIONS_ENABLED)

    if(CLOGGER_STRESS_TEST)
        target_compile_options(${clogger_testlib_target_name} PRIVATE -DCLOGGER_STRESS_TEST)
        target_compile_options(${clogger_testbin_target_name} PRIVATE -DCLOGGER_STRESS_TEST)
    endif()
endif()

if(CLOGGER_BUILD_STATIC)
    add_library(${clogger_static_target_name} STATIC ${logger_src_files})
    target_link_libraries(${clogger_static_target_name} PUBLIC base_target)
    install(TARGETS ${clogger_static_target_name}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(CLOGGER_BUILD_SHARED)
    add_library(${clogger_default_target_name} SHARED ${logger_src_files})
    target_link_libraries(${clogger_default_target_name} PUBLIC base_target)
    install(TARGETS ${clogger_default_target_name}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
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

