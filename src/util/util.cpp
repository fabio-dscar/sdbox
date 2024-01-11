#include <util.h>
#include <fstream>

using namespace sdbox;
using namespace sdbox::util;

namespace fs = std::filesystem;

std::optional<std::string> util::ReadTextFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios_base::in | std::ios_base::ate);
    if (file.fail()) {
        LOG_ERROR("Failed to open file {}. {}", filePath.string(), std::strerror(errno));
        return std::nullopt;
    }

    auto size = file.tellg();
    if (size == -1) {
        LOG_ERROR("Failed to query size of file {}. {}", filePath.string(), std::strerror(errno));
        return std::nullopt;
    }

    std::string contents;
    contents.reserve(size);
    file.seekg(0, std::ios::beg);
    contents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return contents;
}

std::optional<BinaryData> util::ReadBinaryFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios_base::binary | std::ios_base::in | std::ios_base::ate);
    if (file.fail()) {
        LOG_ERROR("Failed to open file {}. {}", filePath.string(), std::strerror(errno));
        return std::nullopt;
    }

    auto size = file.tellg();
    if (size == -1) {
        LOG_ERROR(
            "Failed to query size of binary file {}. {}", filePath.string(), std::strerror(errno));
        return std::nullopt;
    }
    file.seekg(0, std::ios::beg);

    BinaryData bin;
    bin.size = size;
    bin.data = std::make_unique<std::byte[]>(bin.size);
    file.read(reinterpret_cast<char*>(bin.data.get()), bin.size);

    bin.hash     = HashBytes64(bin.data.get(), bin.size);
    bin.filename = filePath.filename();

    return bin;
}