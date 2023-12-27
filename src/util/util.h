#ifndef __UTIL_H__
#define __UTIL_H__

#include <sdbox.h>

#include <optional>
#include <filesystem>

#include <xxhash/xxhash.hpp>
#include <xxhash/constexpr-xxh3.h>

namespace sdbox {
namespace util {

using HashResult64  = std::uint64_t;
using HashResult128 = xxh::uint128_t;

using HashResult = HashResult64;

// ------------------------------------------------------------------
//     General IO
// ------------------------------------------------------------------
std::optional<std::string> ReadTextFile(const std::string& filePath, std::ios_base::openmode mode);

struct BinaryData {
    std::string                  filename;
    HashResult                   hash;
    std::size_t                  size;
    std::unique_ptr<std::byte[]> data;
};

std::optional<BinaryData> ReadBinaryFile(const std::string& filePath);

// ------------------------------------------------------------------
//     Hash functions
// ------------------------------------------------------------------
consteval HashResult64 Hash(const char* bytes, const std::size_t len) {
    return constexpr_xxh3::XXH3_64bits_const(bytes, len);
}

consteval HashResult64 Hash(std::string str) {
    return constexpr_xxh3::XXH3_64bits_const(str);
}

inline constexpr HashResult64 HashBytes64(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<64>(bytes, len);
}

inline constexpr HashResult64 HashBytes64(const std::string& str) {
    return xxh::xxhash3<64>(str);
}

inline HashResult128 HashBytes128(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<128>(bytes, len);
}

inline HashResult128 HashBytes128(const std::string& str) {
    return xxh::xxhash3<128>(str);
}

} // namespace util
} // namespace sdbox

#endif // __UTIL_H__