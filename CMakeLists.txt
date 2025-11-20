cmake_minimum_required(VERSION 3.28)
project(Splay-Trees)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${ASAN_FLAGS}")
    endif()
endif()

add_library(bib STATIC
        include/JobQueue.h
        src/main/JobQueue.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

target_include_directories(bib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#Tests

# FetchContent-Modul laden
include(FetchContent)

# Download google test and configure it
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.12.1
)

FetchContent_MakeAvailable(googletest)

#Function to add a gtest
function(add_gtest test_name)
    add_executable(${test_name} ${ARGN})
    target_link_libraries(${test_name}
            GTest::gtest
            GTest::gtest_main
            bib
    )
    # Add ASan flags to the test target
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${test_name} PRIVATE "-fsanitize=address" "-fno-omit-frame-pointer")
        target_link_options(${test_name} PRIVATE "-fsanitize=address")
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# register tests
enable_testing()

#Add tests
add_gtest(TEST tests/test.cpp)
add_gtest(DEMO demos/demo.cpp)
