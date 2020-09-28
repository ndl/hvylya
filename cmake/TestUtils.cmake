function(addTest TEST_NAME)
    file (GLOB TEST_SOURCES ${TEST_NAME}.cpp)
    set_property (SOURCE ${TEST_SOURCES} PROPERTY LABELS Hvylya)

    add_executable (${TEST_NAME} ${TEST_SOURCES})
    add_test (${TEST_NAME} ${TEST_NAME})

    set_property (TARGET ${TEST_NAME} PROPERTY LABELS Hvylya)
    set_property (TEST ${TEST_NAME} PROPERTY LABELS Hvylya)

    # Adjust load factor, if specified:
    if (TEST_LOAD_FACTOR)
	set_property (TARGET ${TEST_NAME} PROPERTY COMPILE_DEFINITIONS "TEST_LOAD_FACTOR=${TEST_LOAD_FACTOR}")
    else ()
	set_property (TARGET ${TEST_NAME} PROPERTY COMPILE_DEFINITIONS "TEST_LOAD_FACTOR=100")
    endif ()

    add_dependencies (${TEST_NAME} hvylya)
    # Add tracking dependency for our sub-project.
    add_dependencies (Hvylya ${TEST_NAME})

    target_link_libraries (${TEST_NAME} hvylya)
    target_link_libraries (${TEST_NAME} fmt::fmt)
    # Causes memory leak, so disable by default.
    # target_link_libraries (${TEST_NAME} ${GOOGLE_PERF_TOOLS_PROFILER_LIBRARY})
    target_link_libraries (${TEST_NAME} ${GTEST_BOTH_LIBRARIES})
    target_link_libraries (${TEST_NAME} ${GLOG_LIBRARIES})
endfunction()
