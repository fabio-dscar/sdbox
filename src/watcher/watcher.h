#ifndef __WATCHER_H__
#define __WATCHER_H__

#include <sdbox.h>

#include <filesystem>
#include <functional>

namespace sdbox {

class InotifyWatcher;

enum class EventType { FileCreated, FileDeleted, FileChanged, FileRenamed };

struct WatcherEvent {
    std::string name;
    EventType   type;
};

using FileCreatedCallback = std::function<void(const std::string&)>;
using FileDeletedCallback = std::function<void(const std::string&)>;
using FileChangedCallback = std::function<void(const std::string&)>;
using FileRenamedCallback = std::function<void(const std::string&, const std::string&)>;

class DirectoryWatcher {
public:
    DirectoryWatcher(const std::filesystem::path& dirPath) : dirPath(dirPath) {}
    ~DirectoryWatcher() = default;

    virtual bool init()  = 0;
    virtual void watch() = 0;

    void setFileCreatedCallback(FileCreatedCallback callback) { createdCallback = callback; }
    void setFileDeletedCallback(FileDeletedCallback callback) { deletedCallback = callback; }
    void setFileChangedCallback(FileChangedCallback callback) { changedCallback = callback; }
    void setFileRenamedCallback(FileRenamedCallback callback) { renamedCallback = callback; }

protected:
    std::filesystem::path dirPath = "";

    FileCreatedCallback createdCallback = nullptr;
    FileDeletedCallback deletedCallback = nullptr;
    FileChangedCallback changedCallback = nullptr;
    FileRenamedCallback renamedCallback = nullptr;
};

std::unique_ptr<DirectoryWatcher> CreateDirectoryWatcher(const std::filesystem::path& path);

} // namespace sdbox

#endif // __WATCHER_H__