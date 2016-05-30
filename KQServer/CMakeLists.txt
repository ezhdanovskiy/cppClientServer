cmake_minimum_required(VERSION 2.8)
project(KQServer)

add_executable(KQServer KQServer.cpp)
if (PLATFORM_LINUX)
    target_link_libraries(KQServer kqueue)
endif (PLATFORM_LINUX)
