# - Try to find google-perftools libraries, specifically profiler one.
# Once done this will define
#  GOOGLE_PERF_TOOLS_FOUND - the system has google-perftools.
#  GOOGLE_PERF_TOOLS_INCLUDE_DIRS - google-perftools include directories.
#  GOOGLE_PERF_TOOLS_LIBRARIES - all libraries provided by google-perftools.
#  GOOGLE_PERF_TOOLS_PROFILTER_LIBRARY - profiler library from google-perftools.

find_path (GOOGLE_PERF_TOOLS_INCLUDE_DIR gperftools/profiler.h)

find_library (GOOGLE_PERF_TOOLS_PROFILER_LIBRARY NAMES profiler)

set (GOOGLE_PERF_TOOLS_LIBRARIES ${GOOGLE_PERF_TOOLS_PROFILER_LIBRARY})
set (GOOGLE_PERF_TOOLS_INCLUDE_DIRS ${GOOGLE_PERF_TOOLS_INCLUDE_DIR})

include (FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PASTE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args (GooglePerfTools DEFAULT_MSG
                                   GOOGLE_PERF_TOOLS_INCLUDE_DIR
                                   GOOGLE_PERF_TOOLS_PROFILER_LIBRARY)

mark_as_advanced (GOOGLE_PERF_TOOLS_INCLUDE_DIR GOOGLE_PERF_TOOLS_PROFILER_LIBRARY)
