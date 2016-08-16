#pragma once

#include "IOService.h"
#include "Protocol.h"

class Server {
    IOService ioService;
    Protocol protocol;

public:
    int start(int argc, char **argv);
};