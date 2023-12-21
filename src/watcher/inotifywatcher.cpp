#include <watcher/inotifywatcher.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>

using namespace sdbox;

namespace fs = std::filesystem;

constexpr auto PipeRead  = 0;
constexpr auto PipeWrite = 1;

std::optional<EventType> MaskToEventType(std::uint32_t mask) {
    using enum sdbox::EventType;

    if (mask & IN_MOVED_TO)
        return FileMoved;
    else if (mask & IN_CREATE)
        return FileCreated;
    else if (mask & IN_DELETE)
        return FileDeleted;
    else if (mask & IN_CLOSE_WRITE)
        return FileChanged;

    return std::nullopt;
}

void InotifyWatcher::cleanup() {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, inotifyFd, 0);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, stopPipeFd[PipeRead], 0);

    close(inotifyFd);
    close(epollFd);
    close(stopPipeFd[PipeRead]);
    close(stopPipeFd[PipeWrite]);
}

void InotifyWatcher::init() {
    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd == -1)
        FATAL("Failed to initializze inotify. {}", strerror(errno));

    if (pipe2(stopPipeFd.data(), O_NONBLOCK) == -1)
        FATAL("Failed to create stop pipe. {}", strerror(errno));

    // Create notification facility for both descriptors
    epollFd = epoll_create1(0);
    if (epollFd == -1)
        FATAL("Can't create epoll. {}", strerror(errno));

    inotifyEv.data.fd = inotifyFd;
    inotifyEv.events  = EPOLLET | EPOLLIN;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, inotifyFd, &inotifyEv) == -1)
        FATAL("Failed to add inotfy's fd to epoll. {}", strerror(errno));

    stopPipeEv.data.fd = stopPipeFd[PipeRead];
    stopPipeEv.events  = EPOLLET | EPOLLIN;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, stopPipeFd[PipeRead], &stopPipeEv) == -1)
        FATAL("Failed to add stop pipe's fd to epoll. {}", strerror(errno));

    // Add directory to watches
    if (!fs::exists(dirPath))
        FATAL("Path {} does not exist.", dirPath.string());

    if (!fs::is_directory(dirPath))
        FATAL("Provided path is not a directory.");

    watchHandle = inotify_add_watch(inotifyFd, dirPath.c_str(), EventMask);
    if (watchHandle == -1)
        FATAL("Failed to create a watch for {}.", dirPath.string());
}

std::size_t InotifyWatcher::readPoll() {
    epoll_event epollEvent;
    int         numEvents = epoll_wait(epollFd, &epollEvent, 1, -1);

    if (numEvents == -1 || epollEvent.data.fd == stopPipeFd[PipeRead])
        return 0;

    auto sizeRead = read(epollEvent.data.fd, readBuffer.data(), readBuffer.size());
    if (sizeRead == -1) {
        writeToErrorCallback(strerror(errno));
        return 0;
    }

    return sizeRead;
}

std::optional<WatcherEvent> InotifyWatcher::createEvent(const inotify_event& ev) {
    auto type = MaskToEventType(ev.mask);
    if (!type.has_value())
        return std::nullopt;

    WatcherEvent event;
    event.type  = type.value();
    event.name  = ev.name;
    event.isDir = ev.mask & IN_ISDIR;

    if (event.type == EventType::FileMoved) {
        auto it = renameMap.find(ev.cookie);
        if (it != renameMap.end()) {
            event.oldName = it->second;
            renameMap.erase(it);
        }
    }

    return event;
}

bool InotifyWatcher::filterEvent(const inotify_event& ev) {
    std::string fname = ev.name;

    if (ev.mask & IN_IGNORED) {
        {
            std::lock_guard lock{stopMutex};
            stopped = true;
        }

        writeToErrorCallback("Directory got IN_IGNORED; Stopping directory monitoring.");

        return true;
    }

    // Register move/rename
    if (ev.mask & IN_MOVED_FROM) {
        renameMap.emplace(ev.cookie, fname);
        return true;
    }

    // Ignore dotfiles
    if (fname.starts_with("."))
        return true;

    // Ignore temp files created by text editors
    if (fname.ends_with("~"))
        return true;

    // If it has any of the used masks, don't filter
    return !(ev.mask & EventMask);
}

void InotifyWatcher::readFromBuffer(std::size_t size) {
    std::size_t i = 0;
    while (i < size) {
        auto notifEv = reinterpret_cast<inotify_event*>(&readBuffer[i]);

        if (!filterEvent(*notifEv)) {
            auto newEvent = createEvent(*notifEv);
            if (newEvent.has_value())
                eventQueue.push(newEvent.value());
        }

        i += EventStructSize + notifEv->len;
    }
}

void InotifyWatcher::dispatchEvents() {
    while (!eventQueue.empty()) {
        auto ev = eventQueue.front();
        eventQueue.pop();

        auto it = callbacks.find(ev.type);
        if (it != callbacks.end())
            it->second(ev);
    }
}

void InotifyWatcher::watch() {
    while (!hasStopped()) {
        auto len = readPoll();
        readFromBuffer(len);
        dispatchEvents();
    };
}

bool InotifyWatcher::hasStopped() {
    std::lock_guard lock{stopMutex};
    return stopped;
}

void InotifyWatcher::stop() {
    std::lock_guard lock{stopMutex};

    if (!stopped) {
        stopped = true;

        std::uint8_t signal[2] = {1, 0};
        write(stopPipeFd[PipeWrite], signal, sizeof(signal));
    }
}