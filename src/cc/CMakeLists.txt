
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(clogger_cc_src_files
    ${logger_src_files}
    Clogger.cc
)
set_source_files_properties(${clogger_cc_src_files} PROPERTIES LANGUAGE CXX )
set(clogger_cc_shared_output_name "${clogger_shared_output_name}_cc")
set(clogger_cc_static_output_name "${clogger_cc_shared_output_name}_static")

set(cc_clogger_link_lib clogger_cc_shared)
set(cc_clogger_public_header
    "${PROJECT_SOURCE_DIR}/src/cc/Clogger.hpp"
    "${clogger_cc_public_h_location}"
)
if(BUILD_STATIC_LIBS)
    set(cc_clogger_link_lib clogger_cc_static)

    CloggerAddLibraryTarget(
        clogger_cc_static
        STATIC
        "${clogger_cc_src_files}"
        "${cc_clogger_public_header}"
        ${clogger_cc_static_output_name}
    )
endif()
if(BUILD_SHARED_LIBS)
    CloggerAddLibraryTarget(
        clogger_cc_shared
        SHARED
        "${clogger_cc_src_files}"
        "${cc_clogger_public_header}"
        ${clogger_cc_shared_output_name}
    )
endif()

if(CLOGGER_BUILD_TEST)
    add_subdirectory(test)
endif()
