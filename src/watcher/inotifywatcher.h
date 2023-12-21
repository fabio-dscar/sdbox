#ifndef __INOTIFYWATCHER_H__
#define __INOTIFYWATCHER_H__

#include <watcher/watcher.h>

#include <sys/inotify.h>
#include <unistd.h>

namespace sdbox {

constexpr auto EventStructSize = sizeof(inotify_event);
constexpr auto MaxEventSize    = sizeof(inotify_event) + NAME_MAX + 1;

class InotifyWatcher : public DirectoryWatcher {
public:
    InotifyWatcher(const std::filesystem::path& path) : DirectoryWatcher(path) {}

    bool init() override;
    void watch() override;

    void handleEvent(const inotify_event& ev);

private:
    void readAndProcessEvents(const char* buf, std::size_t size);

    int fd          = -1;
    int watchHandle = -1;

    std::map<std::uint32_t, std::string> renameMap{};
};

} // namespace sdbox

#endif // __INOTIFYWATCHER_H__