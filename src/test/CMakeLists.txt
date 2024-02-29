
option(CLOGGER_BUILD_TEST "build the test binary" OFF)

if(CLOGGER_BUILD_TEST)
	set(clogger_test_bin "clogger_test")
	add_executable(clogger_test
		common_util.c
		buffer_suite.c
		format_suite.c
		handler_suite.c
		id_suite.c
		level_suite.c
		logger_suite.c
		msg_suite.c
		test.c
	)
	
	set(clogger_link_lib clogger_shared)
	if(BUILD_STATIC_LIBS)
		set(clogger_link_lib clogger_static)
	endif()
	
	set_target_properties(clogger_test PROPERTIES
		OUTPUT_NAME ${clogger_test_bin}
	)
	
	target_link_libraries(clogger_test PUBLIC
		base_target
		${clogger_link_lib}
		cunit
	)
endif()
