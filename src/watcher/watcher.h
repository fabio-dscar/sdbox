#ifndef SDBOX_WATCHER_H
#define SDBOX_WATCHER_H

#include <sdbox.h>

#include <filesystem>
#include <functional>
#include <ostream>

namespace fs = std::filesystem;

namespace sdbox {

class InotifyWatcher;

enum class EventType { Unknown, FileCreated, FileDeleted, FileChanged, FileMoved };

inline std::ostream& operator<<(std::ostream& stream, const EventType& type) {
    using enum EventType;
    switch (type) {
    case FileCreated:
        stream << "Created";
        break;
    case FileDeleted:
        stream << "Deleted";
        break;
    case FileChanged:
        stream << "Changed";
        break;
    case FileMoved:
        stream << "Moved";
        break;
    case Unknown:
        stream << "Unknown";
        break;
    }

    return stream;
}

struct WatcherEvent {
    std::string dirPath;
    std::string name;
    std::string oldName;
    EventType   type;
    bool        isDir;
};

inline std::ostream& operator<<(std::ostream& stream, const WatcherEvent& ev) {
    stream << std::boolalpha;
    stream << "File " << ev.type << '\n';
    stream << "Name: " << ev.name << '\n';
    if (ev.type == EventType::FileMoved)
        stream << "Old Name: " << ev.oldName << '\n';
    stream << "Is Dir: " << ev.isDir << '\n';
    stream << "Dir: " << ev.dirPath << '\n';

    return stream;
}

using EventCallback = std::function<void(const WatcherEvent&)>;
using ErrorCallback = std::function<void(const std::string&)>;

class DirectoryWatcher {
public:
    DirectoryWatcher()          = default;
    virtual ~DirectoryWatcher() = default;

    virtual void init()  = 0;
    virtual void watch() = 0;
    virtual void stop()  = 0;

    virtual void addDirectory(const fs::path& dirPath)    = 0;
    virtual void removeDirectory(const fs::path& dirPath) = 0;

    void registerCallback(EventType type, EventCallback&& callback) {
        callbacks.emplace(type, std::move(callback));
    }

    void registerCallback(EventType type, const EventCallback& callback) {
        callbacks.emplace(type, callback);
    }

    void registerErrorCallback(ErrorCallback&& callback) { errorCallback = callback; }

protected:
    void writeToErrorCallback(const std::string& errMsg) {
        if (errorCallback)
            errorCallback(errMsg);
    }

    bool hasCallback(EventType type) const { return callbacks.find(type) != callbacks.end(); }

    ErrorCallback                      errorCallback = nullptr;
    std::map<EventType, EventCallback> callbacks;
};

std::unique_ptr<DirectoryWatcher> CreateDirectoryWatcher(const fs::path& dirPath);

} // namespace sdbox

#endif