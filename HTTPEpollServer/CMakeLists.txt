cmake_minimum_required(VERSION 2.8)
project(HTTPEpollServer)

add_executable(HTTPEpollServer
    main.cpp
    Server.cpp
    Controller.cpp
    IOService.cpp
)
