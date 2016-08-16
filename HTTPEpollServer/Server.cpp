#include "defs.h"
#include "Controller.h"
#include "Server.h"

int Server::start(int argc, char **argv) {
    int port = 22000;
    std::string host = "127.0.0.1";
    if (argc >= 3) {
        host = argv[1];
        port = atoi(argv[2]);
    }
    LOG("LISTEN " << host << ":" << port);

    ioService.run(host, port);
}
