#include <util.h>
#include <fstream>

using namespace sdbox;
using namespace sdbox::util;

namespace fs = std::filesystem;

std::optional<std::string>
util::ReadTextFile(const std::string& filePath, std::ios_base::openmode mode) {
    std::ifstream file(filePath, mode);
    if (file.fail()) {
        LOG_ERROR("Failed to open file {}. {}", filePath, std::strerror(errno));
        return std::nullopt;
    }

    std::string contents;
    file.seekg(0, std::ios::end);
    contents.reserve(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);

    contents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return contents;
}

std::optional<BinaryData> util::ReadBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios_base::binary | std::ios_base::in);
    if (file.fail()) {
        LOG_ERROR("Failed to open file {}. {}", filePath, std::strerror(errno));
        return std::nullopt;
    }

    BinaryData bin;

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    if (size == -1) {
        LOG_ERROR("Failed to query size of binary file {}. {}", filePath, std::strerror(errno));
        return std::nullopt;
    }
    file.seekg(0, std::ios::beg);

    bin.size = size;
    bin.data = std::make_unique<std::byte[]>(bin.size);
    file.read(reinterpret_cast<char*>(bin.data.get()), bin.size);

    bin.hash     = HashBytes128(bin.data.get(), bin.size);
    bin.filename = fs::path(filePath).filename();

    return bin;
}

HashResult64 util::HashBytes64(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<64>(bytes, len);
}

HashResult128 util::HashBytes128(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<128>(bytes, len);
}