#ifndef CLIENTSERVER_DEFS_H
#define CLIENTSERVER_DEFS_H

#include <iostream>
#include <err.h>
#include <stdlib.h>

#define LOG(chain) std::cout << chain << "\t" << __FILE__ << ":" << __LINE__ <<std::endl;

#define MAX_EVENTS 10
#define BUFFER_SIZE 1000

enum class FDType {
    events,
    listen,
    client
};

#endif //CLIENTSERVER_DEFS_H
