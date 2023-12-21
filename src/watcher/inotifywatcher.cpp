#include <watcher/inotifywatcher.h>

using namespace sdbox;

bool InotifyWatcher::init() {
    fd = inotify_init();
    if (fd == -1) {
        return false;
    }

    auto mask   = IN_MOVE | IN_CREATE | IN_CLOSE_WRITE | IN_DELETE;
    watchHandle = inotify_add_watch(fd, dirPath.c_str(), mask);
    if (watchHandle == -1) {
        return false;
    }

    return true;
}

void InotifyWatcher::readAndProcessEvents(const char* buf, std::size_t size) {
    const char* ptr = buf;
    while (ptr < buf + size) {
        auto ev = reinterpret_cast<const inotify_event*>(ptr);
        handleEvent(*ev);
        ptr += EventStructSize + ev->len;
    }
}

void InotifyWatcher::watch() {
    char buf[MaxEventSize];

    while (true) {
        auto size = read(fd, buf, MaxEventSize);
        if (size > 0) {
            std::cout << "Read: " << size << '\n';
            readAndProcessEvents(buf, size);
        }
    }
}

void InotifyWatcher::handleEvent(const inotify_event& ev) {
    auto mask = ev.mask;
    if (mask & IN_MOVED_FROM) {
        renameMap.insert({ev.cookie, ev.name});
    } else if (mask & IN_MOVED_TO) {
        auto it = renameMap.find(ev.cookie);
        if (it != renameMap.end()) {
            if (renamedCallback)
                renamedCallback(it->second, ev.name);

            renameMap.erase(it);
        }
    } else if (mask & IN_CLOSE_WRITE) {
        if (changedCallback) {
            changedCallback(ev.name);
        }
    } else if (mask & IN_DELETE) {
        if (deletedCallback) {
            deletedCallback(ev.name);
        }
    } else if (mask & IN_CREATE) {
        if (createdCallback) {
            createdCallback(ev.name);
        }
    }
}