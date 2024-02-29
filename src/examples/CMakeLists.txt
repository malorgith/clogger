cmake_minimum_required(VERSION 3.10)

set(clogger_example_simple "CLOGGER_BUILD_EXAMPLE_SIMPLE")
set(clogger_example_simple_doc "build a simple example program")
set(clogger_example_simple_target "clogger_example_simple")

set(clogger_example_feature "CLOGGER_BUILD_EXAMPLE_FEATURE")
set(clogger_example_feature_doc "build a binary that tests the features of the library")
set(clogger_example_feature_target "clogger_example_feature")

set(clogger_example_stress "CLOGGER_BUILD_EXAMPLE_STRESS")
set(clogger_example_stress_doc "build a binary that can be used to test the performance of the logger")
set(clogger_example_stress_target "clogger_example_stress")

set(clogger_example_cc "CLOGGER_BUILD_EXAMPLE_CC")
set(clogger_example_cc_doc "build an example using c++")
set(clogger_example_cc_target "clogger_example_cc")

set(clogger_install_examples "CLOGGER_INSTALL_EXAMPLES")
set(clogger_install_examples_doc "install the example binaries")

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
MACRO(BUILD_EXAMPLE target src_files link_lib)
    # create the example binary
    add_executable(${target} ${src_files})

    target_link_libraries(${target}
        base_target
        ${link_lib}
    )
    if(${clogger_install_examples})
        install(TARGETS ${target}
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
        )
    endif()
ENDMACRO(BUILD_EXAMPLE)

if(CLOGGER_BUILD_EXAMPLES)
    TOGGLE_OPTION(${clogger_install_examples} ${clogger_install_examples_doc} ON)
    TOGGLE_OPTION(${clogger_example_simple} ${clogger_example_simple_doc} ON)
    TOGGLE_OPTION(${clogger_example_feature} ${clogger_example_feature_doc} ON)
    TOGGLE_OPTION(${clogger_example_stress} ${clogger_example_stress_doc} ON)
    if(CLOGGER_BUILD_CC)
        TOGGLE_OPTION(${clogger_example_cc} ${clogger_example_cc_doc} ON)
    else()
        TOGGLE_OPTION(${clogger_example_cc} ${clogger_example_cc_doc} OFF)
    endif()

    set(CLOGGER_SYMBOL_CHECKS ${CLOGGER_SYMBOL_CHECKS}
        sleep
        PARENT_SCOPE
    )
else()
    TOGGLE_OPTION(${clogger_install_examples} ${clogger_install_examples_doc} OFF)
    TOGGLE_OPTION(${clogger_example_simple} ${clogger_example_simple_doc} OFF)
    TOGGLE_OPTION(${clogger_example_feature} ${clogger_example_feature_doc} OFF)
    TOGGLE_OPTION(${clogger_example_stress} ${clogger_example_stress_doc} OFF)
    if(CLOGGER_BUILD_CC)
        TOGGLE_OPTION(${clogger_example_cc} ${clogger_example_cc_doc} OFF)
    endif()
endif()

set(clogger_example_link_lib clogger_shared)
set(clogger_cc_example_link_lib clogger_cc_shared)
if(BUILD_STATIC_LIBS)
    set(clogger_example_link_lib clogger_static)
    set(clogger_cc_example_link_lib clogger_cc_static)
endif()

if("${${clogger_example_simple}}")
    BUILD_EXAMPLE(${clogger_example_simple_target} "simple_test.c" ${clogger_example_link_lib})
endif()

if("${${clogger_example_feature}}")
    BUILD_EXAMPLE(${clogger_example_feature_target} "feature_test.c" ${clogger_example_link_lib})
endif()

if("${${clogger_example_stress}}")
    BUILD_EXAMPLE(${clogger_example_stress_target} "stress_test.c" ${clogger_example_link_lib})
endif()

if("${${clogger_example_cc}}")
    BUILD_EXAMPLE(${clogger_example_cc_target} "cc_test.cc" ${clogger_cc_example_link_lib})
endif()
