cmake_minimum_required(VERSION 3.13)

project(helsing VERSION 1.2.1
	LANGUAGES
		C
)

if (MSVC)
    set(O_SPEED "/O2")
    set(O_SIZE "/Os")
    set(DEBUG "/Zi")
    add_compile_options(/Wall /W4)
else ()
    set(O_SPEED "-O2")
    set(O_SIZE "-Os")
    set(DEBUG "-g")

    # If the code is underperforming, try recompiling it with some of these flags:
    # -falign-functions=32 -falign-functions=64 -falign-loops=32 -march=native -mtune=native
    add_compile_options(-Wall -Wextra)
endif ()

set(CMAKE_C_STANDARD 23)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
set(MESSAGE "Flags used by the compiler during release builds.")
set(CMAKE_C_FLAGS_RELEASE          "${O_SPEED}"          CACHE STRING ${MESSAGE} FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL       "${O_SIZE}"           CACHE STRING ${MESSAGE} FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "${O_SPEED} ${DEBUG}" CACHE STRING ${MESSAGE} FORCE)

option(ENABLE_LTO "enable link time optimization" OFF)
if (ENABLE_LTO)
    include(CheckIPOSupported)
    cmake_policy(SET CMP0069 NEW)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_SUPPORT_OUTPUT)
    if (IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else ()
        message(WARNING "LTO is not supported: ${IPO_SUPPORT_OUTPUT}")
    endif ()
endif ()

# Re-run cmake if the configurtation files change
function(watch)
    set_property(
            DIRECTORY
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARGV}
    )
endfunction()
watch("${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# Detect if OpenSSL is required
execute_process(
    COMMAND sh -c "${CMAKE_C_COMPILER} \"${CMAKE_CURRENT_SOURCE_DIR}/configuration/output.h\" -dM -E | grep -q \"CHECKSUM_RESULTS\""
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE CHECKSUM
    )
if (CHECKSUM EQUAL "0")
    message(STATUS "OpenSSL required")
    set(HELSING_OPENSSL_CRYPTO "OpenSSL::Crypto")
else ()
    message(STATUS "OpenSSL optional")
    set(HELSING_OPENSSL_CRYPTO "")
endif ()

find_package(Threads)
find_package(OpenSSL)

add_executable(helsing
    src/array/array.c
    src/checkpoint/checkpoint.c
    src/hash/hash.c
    src/helper/helper.c
    src/interval/interval.c
    src/linked_list/llnode.c
    src/main.c
    src/options/options.c
    src/task/task.c
    src/task/taskboard.c
    src/thread/targs.c
    src/thread/targs_handle.c
    src/vampire/cache.c
    src/vampire/vargs.c
    )
target_include_directories(helsing PRIVATE
    .
    configuration
    src/array
    src/checkpoint
    src/hash
    src/helper
    src/interval
    src/linked_list
    src/msentence
    src/options
    src/task
    src/thread
    src/vampire
    )
target_link_libraries(helsing
    Threads::Threads
    ${HELSING_OPENSSL_CRYPTO}
    )
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(helsing ${MATH_LIBRARY})
endif()
find_library(STDTHREADS_LIBRARY stdthreads)
if(STDTHREADS_LIBRARY)
    target_link_libraries(helsing ${STDTHREADS_LIBRARY})
endif()
