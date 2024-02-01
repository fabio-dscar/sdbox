#ifndef SDBOX_INOTIFYWATCHER_H
#define SDBOX_INOTIFYWATCHER_H

#include <watcher/watcher.h>

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <queue>

namespace sdbox {

constexpr auto EventStructSize = sizeof(inotify_event);
constexpr auto MaxEventSize    = sizeof(inotify_event) + NAME_MAX + 1;

constexpr std::uint32_t EventMask =
    IN_MOVE | IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF;

class InotifyWatcher : public DirectoryWatcher {
public:
    InotifyWatcher();
    InotifyWatcher(const fs::path& dirPath);
    ~InotifyWatcher() { cleanup(); }

    void init() override;
    void watch() override;
    void stop() override;

    void addDirectory(const fs::path& dirPath) override;
    void removeDirectory(const fs::path& dirPath) override;

private:
    void removeWatch(int handle);

    std::string getDirPath(int handle) const;
    int         getDirHandle(const std::string& dirPath) const;

    WatcherEvent createEvent(const inotify_event& ev);

    std::size_t readPoll();
    void        readFromBuffer(std::size_t size);
    bool        filterEvent(const inotify_event& ev);
    void        dispatchEvents();

    void setError(int errNo) { error = errNo; }
    bool hasStopped();
    void cleanup();

    std::map<std::uint32_t, std::string> renameMap{};

    std::array<char, MaxEventSize> readBuffer;
    std::queue<WatcherEvent>       eventQueue;

    epoll_event inotifyEv;
    epoll_event stopPipeEv;

    std::array<int, 2> stopPipeFd;

    std::mutex stopMutex;
    bool       stopped = false;

    int inotifyFd = -1;
    int epollFd   = -1;

    std::map<int, std::string> watchHandles;

    int error;
};

} // namespace sdbox

#endif