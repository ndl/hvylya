# Specify sources we build.
file (GLOB FM_RECEIVER_SOURCES *.cpp *.h)
set_property (SOURCE ${FM_RECEIVER_SOURCES} PROPERTY LABELS Hvylya)

add_executable (fm-receiver ${FM_RECEIVER_SOURCES})
set_property (TARGET fm-receiver PROPERTY LABELS Hvylya)

add_dependencies (fm-receiver hvylya)

# Add tracking dependency for our sub-project.
add_dependencies (Hvylya fm-receiver)

include_directories (${FFTW_INCLUDES})
include_directories (${CURSES_INCLUDE_DIRS})

target_link_libraries (fm-receiver hvylya)
target_link_libraries (fm-receiver fmt)
target_link_libraries (fm-receiver ${GOOGLE_PERF_TOOLS_PROFILER_LIBRARY})
target_link_libraries (fm-receiver ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (fm-receiver ${CURSES_LIBRARIES})
target_link_libraries (fm-receiver ${ALSA_LIBRARIES})
target_link_libraries (fm-receiver ${V4L2_LIBRARIES})
target_link_libraries (fm-receiver ${FFTW_LIBRARIES})
target_link_libraries (fm-receiver ${GLOG_LIBRARY})
