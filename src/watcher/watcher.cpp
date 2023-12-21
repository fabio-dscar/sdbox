#include <watcher/watcher.h>

#include <watcher/inotifywatcher.h>

using namespace sdbox;

std::unique_ptr<DirectoryWatcher> sdbox::CreateDirectoryWatcher(const std::filesystem::path& path) {
    return std::make_unique<InotifyWatcher>(path);
}