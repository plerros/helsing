cmake_minimum_required(VERSION 3.13)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_C_FLAGS_RELEASE "-O2" CACHE STRING "Flags used by the compiler during release builds." FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "Flags used by the compiler during release builds." FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL "-Os" CACHE STRING "Flags used by the compiler during release builds." FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os" CACHE STRING "Flags used by the compiler during release builds." FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "Flags used by the compiler during release builds." FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "Flags used by the compiler during release builds." FORCE)

project(helsing VERSION 1.1.1)

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

add_compile_options(-Wall -Wextra)
add_compile_options(-march=native)

# re-run cmake if the configurtation files change

function(watch)
    set_property(
            DIRECTORY
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARGV}
    )
endfunction()

watch("${CMAKE_CURRENT_SOURCE_DIR}/configuration.h")
watch("${CMAKE_CURRENT_SOURCE_DIR}/configuration_adv.h")

# Detect if OpenSSL is required

execute_process(
    COMMAND sh -c "${CMAKE_C_COMPILER} -dM -E -I \"${CMAKE_CURRENT_SOURCE_DIR}configuration.h\" \"${CMAKE_CURRENT_SOURCE_DIR}/configuration_adv.h\" | grep -q \"CHECKSUM_RESULTS\""
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE CHECKSUM
    )

if (CHECKSUM EQUAL "0")
    message(STATUS "OpenSSL required")
    set(HELSING_OPENSSL_CRYPTO "OpenSSL::Crypto")
else()
    message(STATUS "OpenSSL optional")
    set(HELSING_OPENSSL_CRYPTO "")
endif()

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
    src/array
    src/checkpoint
    src/hash
    src/helper
    src/interval
    src/linked_list
    src/options
    src/task
    src/thread
    src/vampire
    )
target_link_libraries(helsing
    m
    Threads::Threads
    ${HELSING_OPENSSL_CRYPTO}
    )
