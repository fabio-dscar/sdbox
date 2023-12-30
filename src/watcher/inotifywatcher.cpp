#include <watcher/inotifywatcher.h>

#include <check.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <util.h>

using namespace sdbox;

constexpr auto PipeRead  = 0;
constexpr auto PipeWrite = 1;

EventType MaskToEventType(std::uint32_t mask) {
    using enum sdbox::EventType;

    if (mask & IN_MOVED_TO)
        return FileMoved;
    else if (mask & IN_CREATE)
        return FileCreated;
    else if (mask & IN_DELETE)
        return FileDeleted;
    else if (mask & IN_CLOSE_WRITE)
        return FileChanged;

    return Unknown;
}

InotifyWatcher::InotifyWatcher() : DirectoryWatcher() {
    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd == -1)
        FATAL("Failed to initialize inotify. {}", std::strerror(errno));
}

InotifyWatcher::InotifyWatcher(const fs::path& dirPath) : InotifyWatcher() {
    addDirectory(dirPath);
}

void InotifyWatcher::cleanup() {
    stop();

    epoll_ctl(epollFd, EPOLL_CTL_DEL, inotifyFd, 0);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, stopPipeFd[PipeRead], 0);

    close(inotifyFd);
    close(epollFd);
    close(stopPipeFd[PipeRead]);
    close(stopPipeFd[PipeWrite]);
}

void InotifyWatcher::init() {
    if (pipe2(stopPipeFd.data(), O_NONBLOCK) == -1)
        FATAL("Failed to create stop pipe. {}", std::strerror(errno));

    // Create notification facility for both pipe and inotify descriptors
    epollFd = epoll_create1(0);
    if (epollFd == -1)
        FATAL("Can't create epoll. {}", std::strerror(errno));

    inotifyEv.data.fd = inotifyFd;
    inotifyEv.events  = EPOLLET | EPOLLIN;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, inotifyFd, &inotifyEv) == -1)
        FATAL("Failed to add inotfy's fd to epoll. {}", std::strerror(errno));

    stopPipeEv.data.fd = stopPipeFd[PipeRead];
    stopPipeEv.events  = EPOLLET | EPOLLIN;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, stopPipeFd[PipeRead], &stopPipeEv) == -1)
        FATAL("Failed to add stop pipe's fd to epoll. {}", std::strerror(errno));
}

void InotifyWatcher::addDirectory(const fs::path& dirPath) {
    if (!fs::exists(dirPath))
        FATAL("Path {} does not exist.", dirPath.string());

    if (!fs::is_directory(dirPath))
        FATAL("Provided path {} is not a directory.", dirPath.string());

    // Add directory to watches
    int handle = inotify_add_watch(inotifyFd, dirPath.c_str(), EventMask);
    if (handle == -1)
        FATAL("Failed to create a watch for {}.", dirPath.string());

    watchHandles[handle] = dirPath;
}

void InotifyWatcher::removeWatch(int handle) {
    if (inotify_rm_watch(inotifyFd, handle) == -1)
        LOG_ERROR("Failed to remove watch for {}.", getDirPath(handle));

    watchHandles.erase(handle);
}

void InotifyWatcher::removeDirectory(const fs::path& dirPath) {
    auto handle = getDirHandle(dirPath);
    if (handle != -1)
        removeWatch(handle);
}

std::size_t InotifyWatcher::readPoll() {
    epoll_event epollEvent;
    int         numEvents = epoll_wait(epollFd, &epollEvent, 1, -1);

    if (numEvents == -1 || epollEvent.data.fd == stopPipeFd[PipeRead])
        return 0;

    auto sizeRead = read(epollEvent.data.fd, readBuffer.data(), readBuffer.size());
    if (sizeRead == -1) {
        writeToErrorCallback(std::strerror(errno));
        return 0;
    }

    return sizeRead;
}

int InotifyWatcher::getDirHandle(const std::string& dirPath) const {
    for (const auto& [handle, path] : watchHandles)
        if (path == dirPath)
            return handle;

    LOG_WARN("Couldn't find watch handle for {}.", dirPath);
    return -1;
}

std::string InotifyWatcher::getDirPath(int handle) const {
    auto it = watchHandles.find(handle);
    if (it != watchHandles.end())
        return it->second;

    LOG_WARN("Trying to query folder to unknown handle");
    return "";
}

WatcherEvent InotifyWatcher::createEvent(const inotify_event& ev) {
    WatcherEvent event;
    event.type    = MaskToEventType(ev.mask);
    event.name    = ev.name;
    event.isDir   = ev.mask & IN_ISDIR;
    event.dirPath = getDirPath(ev.wd);

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

    if (ev.mask & IN_DELETE_SELF) {
        LOGI("Stopped watching {}. Folder was deleted.", getDirPath(ev.wd));
        watchHandles.erase(ev.wd);
        return true;
    }

    if (ev.mask & IN_IGNORED) {
        watchHandles.erase(ev.wd);
        return true;
    }

    // Register move/rename
    if (ev.mask & IN_MOVED_FROM) {
        renameMap.emplace(ev.cookie, fname);
        return true;
    }

    // Ignore dotfiles and temp files created by text editors
    if (fname.starts_with(".") || fname.ends_with("~"))
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
            eventQueue.push(newEvent);
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