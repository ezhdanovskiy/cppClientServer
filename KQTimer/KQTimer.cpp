#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <iostream>
#include <map>

#include <defs.h>

std::map<int, FDType> fdMap;
struct kevent events_to_monitor[MAX_EVENTS];
int events_to_monitor_size = 1;

std::ostream& operator<<(std::ostream &o, const struct kevent &ev);

int main(int argc, char **argv) {
    int events_fd = kqueue();
    if (events_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    struct kevent event;
    EV_SET(&event, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 1000, 0);
    for (int i = 0; i < 5; ++i) {
        int events_size = kevent(events_fd, &event, 1, &event, 1, NULL);
        if (events_size < 0) {
            err(1, "%s:%d", __FILE__, __LINE__);
        }
        if (events_size > 0) {
            if (event.flags & EV_ERROR) {   /* report any error */
                err(1, "%s:%d", __FILE__, __LINE__);
            }
            LOG("time");
        }
    }
    close(events_fd);
}

std::ostream& operator<<(std::ostream &o, const struct kevent &ev) {
    return o << "kevent{fd=" << ev.ident << " filter=" << ev.filter << " flags=" << std::hex << ev.flags << std::dec << "}";
}
