include_directories (${GTEST_INCLUDE_DIR})
include_directories (${FFTW_INCLUDES})

include (TestUtils)

addTest(simd_vector_tests)

addTest(aligned_vector_tests)

addTest(approx_trigonometry_tests)

addTest(lagrange_interpolator_tests)

addTest(running_sum_tests)

addTest(fastsig_tests)

addTest(levinson_tests)
