#ifndef __UTIL_H__
#define __UTIL_H__

#include <sdbox.h>

#include <optional>
#include <filesystem>

#include <xxhash/xxhash.hpp>
#include <xxhash/constexpr-xxh3.h>

namespace fs = std::filesystem;

namespace sdbox {
namespace util {

using HashResult64  = std::uint64_t;
using HashResult128 = xxh::uint128_t;

using HashResult = HashResult64;

// ------------------------------------------------------------------
//     General IO
// ------------------------------------------------------------------
struct BinaryData {
    std::string                  filename;
    HashResult                   hash;
    std::size_t                  size;
    std::unique_ptr<std::byte[]> data;
};

std::optional<std::string> ReadTextFile(const fs::path& filePath);
std::optional<BinaryData>  ReadBinaryFile(const fs::path& filePath);

// ------------------------------------------------------------------
//     Hash functions
// ------------------------------------------------------------------
consteval HashResult64 Hash(const char* bytes, const std::size_t len) {
    return constexpr_xxh3::XXH3_64bits_const(bytes, len);
}

consteval HashResult64 Hash(std::string str) {
    return constexpr_xxh3::XXH3_64bits_const(str);
}

inline HashResult64 HashBytes64(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<64>(bytes, len);
}

inline HashResult64 HashBytes64(const std::string& str) {
    return xxh::xxhash3<64>(str);
}

inline HashResult128 HashBytes128(const std::byte* bytes, std::size_t len) {
    return xxh::xxhash3<128>(bytes, len);
}

inline HashResult128 HashBytes128(const std::string& str) {
    return xxh::xxhash3<128>(str);
}

} // namespace util

// ------------------------------------------------------------------
//     Scoped Enums
// ------------------------------------------------------------------
template<typename T>
requires(std::is_enum_v<T> and requires(T e) { EnableBitmaskOperators(e); })
constexpr auto operator|(const T lhs, const T rhs) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename T>
requires(std::is_enum_v<T> and requires(T e) { EnableBitmaskOperators(e); })
constexpr auto operator&(const T lhs, const T rhs) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename T>
requires(std::is_enum_v<T> and requires(T e) { EnumHasConversion(e); })
constexpr auto ToUnderlying(const T enumVal) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<underlying>(enumVal);
}

} // namespace sdbox

#endif // __UTIL_H__