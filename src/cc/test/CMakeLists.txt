cmake_minimum_required(VERSION 3.10)

set(clogger_cc_test_src_files
    CloggerSuite.cc
)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(clogger_cc_test
    ${clogger_cc_test_src_files}
)

set_target_properties(clogger_cc_test PROPERTIES
    OUTPUT_NAME "clogger_cc_test"
)

target_link_libraries(clogger_cc_test PUBLIC
    base_target
    ${cc_clogger_link_lib}
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(clogger_cc_test)
