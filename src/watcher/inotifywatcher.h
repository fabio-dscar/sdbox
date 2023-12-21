#ifndef __INOTIFYWATCHER_H__
#define __INOTIFYWATCHER_H__

#include <watcher/watcher.h>

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <queue>

namespace sdbox {

constexpr auto EventStructSize = sizeof(inotify_event);
constexpr auto MaxEventSize    = sizeof(inotify_event) + NAME_MAX + 1;

constexpr std::uint32_t EventMask = IN_MOVE | IN_CREATE | IN_CLOSE_WRITE | IN_DELETE;

class InotifyWatcher : public DirectoryWatcher {
public:
    InotifyWatcher(const std::filesystem::path& path) : DirectoryWatcher(path) {}
    ~InotifyWatcher() { cleanup(); }

    void init() override;
    void watch() override;
    void stop() override;

private:
    bool hasStopped();

    std::optional<WatcherEvent> createEvent(const inotify_event& ev);

    std::size_t readPoll();
    void        readFromBuffer(std::size_t size);
    bool        filterEvent(const inotify_event& ev);
    void        dispatchEvents();

    void setError(int errNo) { error = errNo; }

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

    int watchHandle = -1;

    int error;
};

} // namespace sdbox

#endif // __INOTIFYWATCHER_H__