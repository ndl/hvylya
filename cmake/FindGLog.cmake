# - Try to find glog libraries.
# Once done this will define
#  GLOG_FOUND - the system has glog.
#  GLOG_INCLUDE_DIRS - glog include directories.
#  GLOG_LIBRARIES - all libraries provided by glog.

find_path (GLOG_INCLUDE_DIR glog/logging.h)

find_library (GLOG_LIBRARY NAMES glog)

set (GLOG_LIBRARIES ${GLOG_LIBRARY})
set (GLOG_INCLUDE_DIRS ${GLOG_INCLUDE_DIR})

include (FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PASTE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args (GLog DEFAULT_MSG
                                   GLOG_INCLUDE_DIR
                                   GLOG_LIBRARY)

mark_as_advanced (GLOG_INCLUDE_DIR GLOG_IBRARY)
