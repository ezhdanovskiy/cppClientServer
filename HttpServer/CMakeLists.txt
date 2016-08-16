cmake_minimum_required(VERSION 2.8)
project(HttpServer)

add_executable(HttpServer
    main.cpp
    Server.cpp
    Controller.cpp
    IOService.cpp
    Protocol.cpp
)
