#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <defs.h>

std::ostream& operator<<(std::ostream &o, const inotify_event &ev) {
    return o << "inotify_event={"
             << " wd=" << ev.wd
             << " mask=" << std::hex << ev.mask << std::dec
             << " cookie=" << ev.cookie
             << " len=" << ev.len
             << " name='" << ev.name << "'"
             << "}";
}

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int main(int argc, char **argv) {
    char buffer[BUF_LEN];

    int fd = inotify_init();

    if (fd < 0) {
        perror("inotify_init");
    }

    int wd = inotify_add_watch(fd, "/tmp/inotify", IN_MODIFY | IN_CREATE | IN_DELETE);
    LOG1(wd);
    if (wd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    bool notExitFlag = true;
    while (notExitFlag) {
        long length = read(fd, buffer, BUF_LEN);

        if (length < 0) {
            perror("read");
        }
        LOG1(length);
        int i = 0;
        while (i < length) {
            LOG1(i);
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
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
            i += EVENT_SIZE + event->len;
        }
    }
    (void) inotify_rm_watch(fd, wd);
    (void) close(fd);

    exit(0);
}
