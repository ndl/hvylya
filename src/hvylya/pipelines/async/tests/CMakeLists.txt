include (TestUtils)

include_directories (${GTEST_INCLUDE_DIR})

addTest(pipeline_tests)
target_link_libraries (pipeline_tests ${CMAKE_THREAD_LIBS_INIT})

addTest(fm_receiver_tests)
target_link_libraries (fm_receiver_tests ${FFTW_LIBRARIES})
target_link_libraries (fm_receiver_tests ${CMAKE_THREAD_LIBS_INIT})

addTest(rds_messages_decoder_tests)
target_link_libraries (rds_messages_decoder_tests ${CMAKE_THREAD_LIBS_INIT})
