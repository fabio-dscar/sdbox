#ifndef __THREAD_H__
#define __THREAD_H__

#include <string>

namespace sdbox {

void               SetThreadName(const std::string& name);
const std::string& GetThreadName();

} // namespace sdbox

#endif // __THREAD_H__