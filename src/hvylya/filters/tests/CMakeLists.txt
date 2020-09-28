include (TestUtils)

include_directories (${GTEST_INCLUDE_DIR})
include_directories (${FFTW_INCLUDES})

addTest(fir_filter_tests)

addTest(fir_filter_long_tests)
set_property (TEST fir_filter_long_tests APPEND PROPERTY LABELS Long)

addTest(fft_filter_tests)
target_link_libraries (fft_filter_tests ${FFTW_LIBRARIES})

addTest(fft_filter_long_tests)
target_link_libraries (fft_filter_long_tests ${FFTW_LIBRARIES})
set_property (TEST fft_filter_long_tests APPEND PROPERTY LABELS Long)

addTest(resampler_tests)

addTest(resampler_long_tests)
set_property (TEST resampler_long_tests APPEND PROPERTY LABELS Long)

addTest(rds_bits_corrector_tests)

addTest(mapper_filter_tests)

addTest(pm_optimizer_tests)

addTest(pm_filters_designer_tests)

addTest(iir_filters_designer_tests)
