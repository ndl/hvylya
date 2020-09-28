# Specify sources we build.
file (GLOB FILTERS_DESIGNER_SOURCES filters_designer.cpp)

add_executable (filters-designer ${FILTERS_DESIGNER_SOURCES})

add_dependencies (filters-designer hvylya)
# Add tracking dependency for our sub-project.
add_dependencies (Hvylya filters-designer)

target_link_libraries (filters-designer hvylya)
target_link_libraries (filters-designer ${FFTW_LIBRARIES})
target_link_libraries (filters-designer ${GOOGLE_PERF_TOOLS_PROFILER_LIBRARY})
target_link_libraries (filters-designer ${GLOG_LIBRARY})

file (GLOB FIR_EXP_SOURCES fir_exp.cpp)

add_executable (fir-exp ${FIR_EXP_SOURCES})

add_dependencies (fir-exp hvylya)
# Add tracking dependency for our sub-project.
add_dependencies (Hvylya fir-exp)

target_link_libraries (fir-exp hvylya)
target_link_libraries (fir-exp ${FFTW_LIBRARIES})
target_link_libraries (fir-exp ${GOOGLE_PERF_TOOLS_PROFILER_LIBRARY})
target_link_libraries (fir-exp ${GLOG_LIBRARY})
