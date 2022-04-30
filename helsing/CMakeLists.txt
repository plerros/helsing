cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(helsing VERSION 1.1.1)

option(ENABLE_LTO "enable link time optimization" OFF)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    cmake_policy(SET CMP0069 NEW)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_SUPPORT_OUTPUT)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(WARNING "LTO is not supported: ${IPO_SUPPORT_OUTPUT}")
    endif()
endif()

#add_compile_options(-Wall -Wextra)
add_compile_options(-march=native)

string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

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
    OpenSSL::Crypto
    )