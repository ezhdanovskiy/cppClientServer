#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <defs.h>
#include <sys/epoll.h>

std::ostream& operator<<(std::ostream &o, const inotify_event &ev) {
    return o << "inotify_event={"
             << " wd=" << ev.wd
             << " mask=" << std::hex << ev.mask << std::dec
             << " cookie=" << ev.cookie
             << " len=" << ev.len
             << " name='" << ev.name << "'"
             << " }";
}

#define INOTIFY_EVENT_SIZE      ( sizeof (struct inotify_event) )
#define INOTIFY_EVENTS_BUF_LEN  ( 1024 * ( INOTIFY_EVENT_SIZE + 16 ) )

int main(int argc, char **argv) {
    int inotify_fd = inotify_init();
    CHECK_INT_ERR(inotify_fd);

    int wd = inotify_add_watch(inotify_fd, "/tmp/inotify/1.txt", IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE | IN_MOVE_SELF);
    CHECK_INT_ERR(wd);

    int events_fd = epoll_create1(0);
    CHECK_INT_ERR(events_fd);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.data.fd = inotify_fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(events_fd, EPOLL_CTL_ADD, inotify_fd, &ev)) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

    char inotify_events[INOTIFY_EVENTS_BUF_LEN];
    bool notExitFlag = true;
    while (notExitFlag) {
        const int infinity = -1;
        int event_size = epoll_wait(events_fd, events, MAX_EVENTS, infinity);
        CHECK_INT_ERR(event_size);
        for (int i = 0; i < event_size; ++i) {
            long length = read(events[i].data.fd, inotify_events, INOTIFY_EVENTS_BUF_LEN);
            CHECK_LONG_ERR(length);
            LOG1(length);
            int inotify_events_pos = 0;
            while (inotify_events_pos < length) {
                LOG1(inotify_events_pos);
                struct inotify_event *ievent = (struct inotify_event *) &inotify_events[inotify_events_pos];
                LOG(*ievent);

                if (ievent->mask & (IN_IGNORED | IN_MOVE_SELF)) {
                    notExitFlag = false;
                }

                if (ievent->len) {
                    if (ievent->mask & IN_CREATE) {
                        if (ievent->mask & IN_ISDIR) {
                            LOG("The directory '" << ievent->name << "' was created.");
                        }
                        else {
                            LOG("The file " << ievent->name << " was created.");
                        }
                    }
                    else if (ievent->mask & IN_DELETE) {
                        if (ievent->mask & IN_ISDIR) {
                            LOG("The directory '" << ievent->name << "' was deleted.");
                        }
                        else {
                            LOG("The file '" << ievent->name << "' was deleted.");
                        }
                    }
                    else if (ievent->mask & IN_MODIFY) {
                        if (ievent->mask & IN_ISDIR) {
                            LOG("The directory '" << ievent->name << "' was modified.");
                        }
                        else {
                            LOG("The file '" << ievent->name << "' was modified.");
                        }
                    }
                }
                inotify_events_pos += INOTIFY_EVENT_SIZE + ievent->len;
            }
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    close(events_fd);

    return 0;
}
