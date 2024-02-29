cmake_minimum_required(VERSION 3.10)

option(CLOGGER_ENABLE_GRAYLOG "build support for a Graylog handler" ON)
option(CLOGGER_NO_SLEEP "don't sleep while waiting for messages; increases CPU usage" OFF)
option(CLOGGER_ADD_MACROS "include macros in the public header" OFF)

set(BUILD_GRAYLOG_HANDLER "")
set(BUILD_CLOGGER_SLEEP "")
set(BUILD_CLOGGER_MACROS "")

if(CLOGGER_ENABLE_GRAYLOG)
    set(logger_src_files ${logger_src_files} ${PROJECT_SOURCE_DIR}/src/handlers/graylog_handler.c PARENT_SCOPE)
    target_compile_definitions(base_target INTERFACE CLOGGER_GRAYLOG)

    set(CLOGGER_INCLUDE_FILES ${CLOGGER_INCLUDE_FILES}
        arpa/inet.h
        netdb.h
        sys/socket.h
		PARENT_SCOPE
    )

#   check_type_size(addrinfo SIZE_ADDRINFO)

    set(CLOGGER_SYMBOL_CHECKS ${CLOGGER_SYMBOL_CHECKS}
        getaddrinfo
        freeaddrinfo
        gai_strerror
        memset
        gethostname
        close
		PARENT_SCOPE
    )

    file(READ graylog_handler_public.h.in BUILD_GRAYLOG_HANDLER)
endif()

if(CLOGGER_NO_SLEEP)
    target_compile_options(base_target INTERFACE -DCLOGGER_NO_SLEEP)
else()
    file(READ clogger_sleep_public.h.in BUILD_CLOGGER_SLEEP)
endif()

if(CLOGGER_ADD_MACROS)
    file(READ clogger_macro_defs_public.h.in BUILD_CLOGGER_MACROS)
endif()

#configure_file("${PROJECT_SOURCE_DIR}/src/clogger.h.in" "${CMAKE_BINARY_DIR}/generated/clogger.h" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/clogger.h.in" "${clogger_public_h_location}" @ONLY)

if(CLOGGER_BUILD_CC)
    file(READ clogger_namespace_open_public.h.in BUILD_CLOGGER_NAMESPACE_OPEN)
    file(READ clogger_namespace_close_public.h.in BUILD_CLOGGER_NAMESPACE_CLOSE)
#   configure_file("${PROJECT_SOURCE_DIR}/src/clogger.h.in" "${CMAKE_BINARY_DIR}/generated/clogger.hpp" @ONLY)
    configure_file("${PROJECT_SOURCE_DIR}/src/clogger.h.in" "${clogger_cc_public_h_location}" @ONLY)
endif()
