cmake_minimum_required(VERSION 2.8)
project(KQTimer)

add_executable(KQTimer KQTimer.cpp)
if (PLATFORM_LINUX)
    target_link_libraries(KQTimer kqueue)
endif (PLATFORM_LINUX)
