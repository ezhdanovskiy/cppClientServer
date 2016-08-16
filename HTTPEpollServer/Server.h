#pragma once

#include "IOService.h"

class Server {
    IOService ioService;

public:
    int start(int argc, char **argv);
};