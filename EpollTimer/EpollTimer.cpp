#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <err.h>
#include <map>
#include <sys/timerfd.h>

#include <defs.h>

std::ostream& operator<<(std::ostream &o, const epoll_event &ev);

int main(int argc, char **argv) {
    int events_fd = epoll_create1(0);
    if (events_fd < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

    int tfd1 = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (tfd1 < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    int tfd2 = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (tfd2 < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    itimerspec new_timeout;
    new_timeout.it_value.tv_sec = 2;
    new_timeout.it_value.tv_nsec = 0;
    new_timeout.it_interval.tv_sec = 2;
    new_timeout.it_interval.tv_nsec = 0;
    if(timerfd_settime(tfd1, 0, &new_timeout, NULL) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    new_timeout.it_value.tv_sec = 5;
    new_timeout.it_value.tv_nsec = 0;
    new_timeout.it_interval.tv_sec = 5;
    new_timeout.it_interval.tv_nsec = 0;
    if(timerfd_settime(tfd2, 0, &new_timeout, NULL) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.data.fd = tfd1;
    ev.events = EPOLLIN;
    if(epoll_ctl(events_fd, EPOLL_CTL_ADD, tfd1, &ev) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    ev.data.fd = tfd2;
    ev.events = EPOLLIN;
    if(epoll_ctl(events_fd, EPOLL_CTL_ADD, tfd2, &ev) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    for (int i = 0; i < 5; ++i) {
        const int infinity = -1;
        int events_size = epoll_wait(events_fd, events, MAX_EVENTS, infinity);
        if (events_size < 0) {
            err(1, "%s:%d", __FILE__, __LINE__);
        }
        for (int i = 0; i < events_size; ++i) {
            size_t s = 0;
            int res = read(events[i].data.fd, &s, sizeof(s));
            if (res != -1) {
                std::cout << "Timer triggered (" << s << ")\n";
                // handle_event();
            }
            LOG("time " << events[i] << " time=" << time(0));
        }
    }

    close(tfd1);
    close(tfd2);
    close(events_fd);
}
std::ostream& operator<<(std::ostream &o, const epoll_event &ev) {
    return o << "epoll_event{fd=" << ev.data.fd << " events=" << std::hex << ev.events << std::dec << "}";
}
