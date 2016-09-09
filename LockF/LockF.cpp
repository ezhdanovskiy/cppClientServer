#include "defs.h"
#include "Logger.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>


int main(int argc, char **argv) {
    std::string fname("file.lock");
    int mode = 0666;
    int fd;
    switch (fork()) {
        case -1: {
            err(1, "fork");
        }
        case 0: {
            usleep(100000);
            int flags = O_RDWR | O_CREAT;
//            int flags = O_RDONLY | O_CREAT; // do not lock on linux
            fd = ::open(fname.c_str(), flags, mode);
            LOG("CHILD  ::open('" << fname << "', " << flags << ", " << mode << ") return " << fd);
            if (-1 == fd) {
                LOG("open error(" << errno << "): " << strerror(errno));
            }

            LOG("before F_LOCK")
            int res = lockf(fd, F_LOCK, 0);
            if (-1 == res) {
                LOG("open error(" << errno << "): " << strerror(errno));
            }

            LOG("after lock")
            break;
        }
        default: {
            int flags = O_RDWR | O_CREAT;
            fd = ::open(fname.c_str(), flags, mode);
            LOG("PARENT ::open('" << fname << "', " << flags << ", " << mode << ") return " << fd);
            if (-1 == fd) {
                LOG("open error(" << errno << "): " << strerror(errno));
            }

            LOG("before F_LOCK")
            int res = lockf(fd, F_LOCK, 0);
            if (-1 == res) {
                LOG("open error(" << errno << "): " << strerror(errno));
            }

            LOG("after lock")

            sleep(3);
            break;
        }
    }
    LOG("F_ULOCK");
    close(fd);
    LOG("close(" << fd << ")");
    close(fd);
    return 0;
}
