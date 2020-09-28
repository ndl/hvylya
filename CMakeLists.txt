cmake_minimum_required (VERSION 3.15)

project (Hvylya)

enable_testing ()

# Make sure cmake can find our custom modules.
list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Adjust the settings that are specific to the local machine.
include (cmake/LocalConfig.cmake)

if (ENABLE_STATIC_ANALYZER)
    set (CMAKE_C_COMPILER "ccc-analyzer")
    set (CMAKE_CXX_COMPILER "c++-analyzer")
    # Disable all optimizations for better performance, as ironic as it might seem ;-)
    set (OPTIMIZATION_LEVEL "-O0")
    # Tell anyone who might care that we're building in Debug mode.
    add_definitions (-DDEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
else ()
    # Current code requires clang to compile due to __builtin_shufflevector.
    set (CMAKE_C_COMPILER "clang")
    set (CMAKE_CXX_COMPILER "clang++")

    if (ENABLE_COVERAGE)
        # The reason we're generating gcov-compatible results & using gcov is that llvm-cov fails to read
        # clang-generated results with default options.
        # TODO: figure out if there's a way to generate coverage info that
        # llvm-cov would be able to read.
        string (CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
            "--coverage " # Enable coverage
            "-Qunused-arguments " # The following arguments cause issues with linking if this warning is not silenced
            "-Xclang -coverage-cfg-checksum " # Generate GCOV 4.9.x compatible results
            "-Xclang -coverage-no-function-names-in-data " # See above
            "-Xclang -coverage-version='409*'" # See above
        )
        # Disable all optimizations so that the coverage results are more accurate.
        set (OPTIMIZATION_LEVEL "-O0")
        # Tell anyone who might care that we're building in Debug mode.
        add_definitions (-DDEBUG)
    elseif (ENABLE_DEBUG)
        # Enable full compiler and link-time optimization.
        set (OPTIMIZATION_LEVEL "-O3 -flto")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
        # Tell anyone who might care that we're building in Debug mode.
        add_definitions (-DDEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
    else ()
        # Enable full compiler and link-time optimization.
        set (OPTIMIZATION_LEVEL "-O3 -flto")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
        # Tell anyone who might care that we're building in Release mode.
        add_definitions (-DNDEBUG)
    endif ()
endif ()

if (NOT USE_ARCH)
    set (USE_ARCH "native")
endif ()

string (CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
    "-std=c++17 "
    "-march=${USE_ARCH} "
    "-fcolor-diagnostics "
    "-g "
    "${OPTIMIZATION_LEVEL}"
)

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OPTIMIZATION_LEVEL}")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OPTIMIZATION_LEVEL}")
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${OPTIMIZATION_LEVEL}")

# Tune Clang warnings:
string (CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
    "-Weverything " # Enable all possible warnings
    "-Werror " # ... and turn them into errors
    "-Wno-c++98-compat " # Disable C++ 98 compatibility warnings - we explicitly target C++ 17
    "-Wno-c++98-compat-pedantic " # Same as above
    "-Wno-double-promotion " # Should be harmless, mostly might affect performance
    "-Wno-implicit-int-float-conversion " # Kind of makes sense, but complicates the code - still need to decide
    "-Wno-padded " # These are not actionable, we do want to have this padding most of the time
    "-Wno-undefined-func-template " # We explicitly instantiate majority of the templates in different compilation units
    "-Wno-weak-template-vtables " # Cannot be fixed at all, just FYI warning
    "-Wno-weak-vtables " # The increased complexitiy of fixing this is not worth the trouble
)

# Set sanitization options, if enabled:
if (ENABLE_ADDRESS_SANITIZER)
    string (CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
        "-fsanitize=address,undefined,vptr" # UBSAN can be combined with other sanitizers
    )
elseif (ENABLE_MEMORY_SANITIZER)
    string (CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
        "-fsanitize=memory "
        "-fsanitize-memory-track-origins"
    )
elseif (ENABLE_THREAD_SANITIZER)
    string (CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
        "-fsanitize=thread"
    )
endif ()

find_package (ALSA REQUIRED)
find_package (Curses REQUIRED)
find_package (FFTW REQUIRED)
find_package (fmt REQUIRED)
find_package (GLog REQUIRED)
find_package (GTest REQUIRED)
find_package (GooglePerfTools REQUIRED)
find_package (Threads REQUIRED)
find_package (V4L2 REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/third-party)
include_directories (${GLOG_INCLUDE_DIRS})

add_custom_target (Hvylya)
set_property (TARGET Hvylya PROPERTY LABELS Hvylya)

add_subdirectory (src/hvylya)
add_subdirectory (src/fm-receiver)
add_subdirectory (src/utils)
