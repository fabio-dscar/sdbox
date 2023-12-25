#ifndef __UTIL_H__
#define __UTIL_H__

#include <sdbox.h>

#include <optional>
#include <filesystem>

#include <xxhash/xxhash.hpp>

namespace sdbox {
namespace util {

using HashResult64  = std::uint64_t;
using HashResult128 = xxh::uint128_t;

using HashResult = HashResult128;

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
HashResult64  HashBytes64(const std::byte* bytes, std::size_t len);
HashResult128 HashBytes128(const std::byte* bytes, std::size_t len);

} // namespace util
} // namespace sdbox

#endif // __UTIL_H__