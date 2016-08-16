#pragma once

class HTTPServer {
    int events_fd;
public:
    int start(int argc, char **argv);
};