cmake_minimum_required(VERSION 3.10)

set(clogger_example_simple "CLOGGER_BUILD_EXAMPLE_SIMPLE")
set(clogger_example_simple_doc "build a simple example program")
set(clogger_example_simple_target "${clogger_default_target_name}_example_simple")

set(clogger_example_feature "CLOGGER_BUILD_EXAMPLE_FEATURE")
set(clogger_example_feature_doc "build a binary that tests the features of the library")
set(clogger_example_feature_target "${clogger_default_target_name}_example_feature")

set(clogger_example_stress "CLOGGER_BUILD_EXAMPLE_STRESS")
set(clogger_example_stress_doc "build a binary that can be used to test the performance of the logger")
set(clogger_example_stress_target "${clogger_default_target_name}_example_stress")

# macro to toggle an option's availability
MACRO(TOGGLE_OPTION option opt_doc enabled)
    if(${enabled})
        option(${option} "${opt_doc}" OFF)
        set(${option} "${${option}}" CACHE BOOL "${opt_doc}" FORCE)
    else()
        set(${option} "${${option}}" CACHE INTERNAL "${opt_doc}")
        set(${option} OFF)
    endif()
ENDMACRO(TOGGLE_OPTION)

# macro to build an example executable
MACRO(BUILD_EXAMPLE target src_files)
    # create the example binary
    add_executable(${target} ${src_files})
    target_include_directories(${target} PRIVATE "../")
    # set the output location for the test binary
    set_target_properties(${target} PROPERTIES
        # TODO is there a better place to put these? don't want to install them
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/${CMAKE_INSTALL_BINDIR}"
    )

    target_link_directories(${target} PRIVATE ${CMAKE_BINARY_DIR})

    set(clogger_example_link_lib ${clogger_shared_target})
    if(CLOGGER_BUILD_STATIC)
        # link to the static lib so we can get debug symbols
        set(clogger_example_link_lib ${clogger_static_target})
    endif()

    target_link_libraries(${target}
        ${clogger_example_link_lib}
    )
ENDMACRO(BUILD_EXAMPLE)

if(CLOGGER_BUILD_EXAMPLES)
    TOGGLE_OPTION(${clogger_example_simple} ${clogger_example_simple_doc} ON)
    TOGGLE_OPTION(${clogger_example_feature} ${clogger_example_feature_doc} ON)
    TOGGLE_OPTION(${clogger_example_stress} ${clogger_example_stress_doc} ON)

    # TODO the code below should be added if the appropriate example(s) are enabled
#   set(CLOGGER_SYMBOL_CHECKS ${CLOGGER_SYMBOL_CHECKS}
#       sleep
#   )
else()
    TOGGLE_OPTION(${clogger_example_simple} ${clogger_example_simple_doc} OFF)
    TOGGLE_OPTION(${clogger_example_feature} ${clogger_example_feature_doc} OFF)
    TOGGLE_OPTION(${clogger_example_stress} ${clogger_example_stress_doc} OFF)
endif()

if("${${clogger_example_simple}}")
    BUILD_EXAMPLE(${clogger_example_simple_target} "simple_test.c")
endif()

if("${${clogger_example_feature}}")
    BUILD_EXAMPLE(${clogger_example_feature_target} "feature_test.c")
endif()

if("${${clogger_example_stress}}")
    BUILD_EXAMPLE(${clogger_example_stress_target} "stress_test.c")
endif()

