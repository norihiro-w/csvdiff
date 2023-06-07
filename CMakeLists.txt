cmake_minimum_required (VERSION 2.8.12)
project(csvdiff)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra")
endif()

include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/tclap/include)

add_executable(csvdiff csvdiff.cpp)
