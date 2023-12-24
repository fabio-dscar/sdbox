#ifndef __UTIL_H__
#define __UTIL_H__

#include <sdbox.h>

#include <optional>
#include <filesystem>

namespace sdbox {
namespace util {

using HashResult = unsigned long;

// ------------------------------------------------------------------
//     General IO
// ------------------------------------------------------------------
std::optional<std::string> ReadTextFile(const std::string& filePath, std::ios_base::openmode mode);

HashResult HashBytes(const std::byte* bytes, std::size_t len);

} // namespace util
} // namespace sdbox

#endif // __UTIL_H__