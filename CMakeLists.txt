cmake_minimum_required(VERSION 2.8)
project(ClientServer)

include(cmake/IdentifyPlatform.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(SimpleClientServer)

if (PLATFORM_LINUX)
    add_subdirectory(EpollServer)
endif (PLATFORM_LINUX)

add_subdirectory(KQServer)
add_subdirectory(KQTimer)
