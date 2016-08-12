#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <iostream>
#include <map>

#include <defs.h>

std::ostream& operator<<(std::ostream &o, const struct kevent &ev) {
    return o << "kevent{fd=" << ev.ident << " filter=" << ev.filter << " flags=" << std::hex << ev.flags << std::dec << "}";
}

int main(int argc, char **argv) {
    int events_fd = kqueue();
    if (events_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    struct kevent events[MAX_EVENTS];
    EV_SET(&events[0], 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 5000, 0);
    EV_SET(&events[1], 2, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 2000, 0);
    kevent(events_fd, events, 2, NULL, 0, NULL);
    for (int i = 0; i < 5; ++i) {
        int events_size = kevent(events_fd, NULL, 0, events, MAX_EVENTS, NULL);
        if (events_size < 0) {
            err(1, "%s:%d", __FILE__, __LINE__);
        }
        for (int i = 0; i < events_size; ++i) {
            if (events[i].flags & EV_ERROR) {   /* report any error */
                err(1, "%s:%d", __FILE__, __LINE__);
            }
            LOG("time " << events[i] << " time=" << time(0));
        }
    }
    close(events_fd);
}
