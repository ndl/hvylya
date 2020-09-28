# - Try to find fftw libraries.
# Once done this will define
#  FFTW_FOUND - the system has fftw.
#  FFTW_INCLUDE_DIRS - fftw include directories.
#  FFTW_LIBRARIES - all libraries provided by fftw.
#  FFTWF_LIBRARY - single-precision FFTW library.
#  FFTW_LIBRARY - double-precision FFTW library.
#  FFTWL_LIBRARY - long double-precision FFTW library.

find_path (FFTW_INCLUDE_DIR fftw3.h)

find_library (FFTW_LIBRARY NAMES fftw3)
find_library (FFTWF_LIBRARY NAMES fftw3f)
find_library (FFTWL_LIBRARY NAMES fftw3l)

set (FFTW_LIBRARIES ${FFTW_LIBRARY} ${FFTWF_LIBRARY} ${FFTWL_LIBRARY})
set (FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR})

include (FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PASTE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args (FFTW DEFAULT_MSG
                                   FFTW_INCLUDE_DIR
                                   FFTW_LIBRARY FFTWF_LIBRARY FFTWL_LIBRARY)

mark_as_advanced (FFTW_INCLUDE_DIR FFTW_LIBRARY FFTWF_LIBRARY FFTWL_LIBRARY)
