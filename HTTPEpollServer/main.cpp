#include "HTTPEpollServer.h"

int main(int argc, char **argv) {
    HTTPServer server;
    return server.start(argc, argv);
}