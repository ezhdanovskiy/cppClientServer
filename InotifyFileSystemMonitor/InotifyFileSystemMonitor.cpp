#include "defs.h"
#include "Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>

std::ostream& operator<<(std::ostream &o, const inotify_event &ev) {
    return o << "inotify_event={"
             << " wd=" << ev.wd
             << " mask=" << std::hex << ev.mask << std::dec
             << " cookie=" << ev.cookie
             << " len=" << ev.len
             << " name='" << ev.name << "'"
             << "}";
}

#define INOTIFY_EVENT_SIZE  ( sizeof (struct inotify_event) )
#define INOTIFY_EVENTS_BUF_LEN     ( 1024 * ( INOTIFY_EVENT_SIZE + 16 ) )

int main(int argc, char **argv) {
    int inotify_fd = inotify_init();
    CHECK_INT_ERR(inotify_fd);

    int wd = inotify_add_watch(inotify_fd, "/tmp/inotify", IN_MODIFY | IN_CREATE | IN_DELETE);
    CHECK_INT_ERR(wd);
    LOG1(wd);

    char inotify_events[INOTIFY_EVENTS_BUF_LEN];
    bool notExitFlag = true;
    while (notExitFlag) {
        long length = read(inotify_fd, inotify_events, INOTIFY_EVENTS_BUF_LEN);
        CHECK_LONG_ERR(length);
        LOG1(length);
        int inotify_events_pos = 0;
        while (inotify_events_pos < length) {
            LOG1(inotify_events_pos);
            struct inotify_event *event = (struct inotify_event *) &inotify_events[inotify_events_pos];
            LOG(*event);

            if (event->mask & IN_IGNORED) {
                notExitFlag = false;
            }

            if (event->len) {
                if (event->mask & IN_CREATE) {
                    if (event->mask & IN_ISDIR) {
                        LOG("The directory '" << event->name << "' was created.");
                    }
                    else {
                        LOG("The file " << event->name << " was created.");
                    }
                }
                else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        LOG("The directory '" << event->name << "' was deleted.");
                    }
                    else {
                        LOG("The file '" << event->name << "' was deleted.");
                    }
                }
                else if (event->mask & IN_MODIFY) {
                    if (event->mask & IN_ISDIR) {
                        LOG("The directory '" << event->name << "' was modified.");
                    }
                    else {
                        LOG("The file '" << event->name << "' was modified.");
                    }
                }
            }
            inotify_events_pos += INOTIFY_EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);

    return 0;
}
