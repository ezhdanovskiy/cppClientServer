#pragma once

#include <string>

class IOService {
public:
    IOService();

    virtual ~IOService();

    int run(std::string host, int port);

//private:
    int fdEvents;
};


