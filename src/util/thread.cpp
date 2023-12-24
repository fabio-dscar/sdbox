#include <thread.h>

using namespace sdbox;

static thread_local std::string ThreadName = "unnamed";

void sdbox::SetThreadName(const std::string& name) {
    ThreadName = name;
}

const std::string& sdbox::GetThreadName() {
    return ThreadName;
}