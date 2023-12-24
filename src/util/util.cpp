#include <util.h>
#include <fstream>

#include <xxhash/xxhash.hpp>

using namespace sdbox;
using namespace sdbox::util;

std::optional<std::string>
util::ReadTextFile(const std::string& filepath, std::ios_base::openmode mode) {
    std::ifstream file(filepath, mode);
    if (file.fail()) {
        perror(filepath.c_str());
        return std::nullopt;
    }

    std::string contents;
    file.seekg(0, std::ios::end);
    contents.reserve(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);

    contents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return contents;
}

HashResult util::HashBytes(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<64>(bytes, len);
}