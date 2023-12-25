#ifndef __SDBOX_BUFFER_H__
#define __SDBOX_BUFFER_H__

#include <glad/glad.h>
#include <sdbox.h>

namespace sdbox {

enum class BufferType : unsigned int { Array = 0, Element = 1, Uniform = 2 };
enum class BufferFlag : unsigned int {
    Dynamic       = 0x1,
    Read          = 0x2,
    Write         = 0x4,
    Persistent    = 0x8,
    Coherent      = 0x16,
    ClientStorage = 0x32
};

// In nanoseconds
static constexpr unsigned long FenceTimeout = 1.0 / 30.0 * 1e9;

class Buffer {
public:
    Buffer() = default;
    Buffer(BufferType type, std::size_t size, unsigned int flags, void* data = nullptr);
    ~Buffer();

    void create(BufferType type, std::size_t size, unsigned int flags, void* data);
    void bindRange(unsigned int index, std::size_t offset, std::size_t size) const;

    template<typename T>
    T* get(std::size_t offset = 0) const {
        return reinterpret_cast<T*>(ptr + offset);
    }

protected:
    unsigned int target = 0;
    unsigned int handle = 0;

private:
    std::byte*   ptr   = nullptr;
    std::size_t  size  = 0;
    unsigned int flags = 0;
};

// --------------------------------------------------------------------------------------
//      Synchronized Buffer
// --------------------------------------------------------------------------------------
struct BufferRangeLock {
    GLsync      lock  = 0;
    std::size_t start = 0;
    std::size_t size  = 0;

    bool overlaps(std::size_t rhsStart, std::size_t rhsSize) {
        return (start + size) > rhsStart && (rhsStart + rhsSize) > start;
    }
};

class SyncedBuffer : public Buffer {
public:
    SyncedBuffer() = default;

    void waitRange(std::size_t start, std::size_t size);
    void lockRange(std::size_t start, std::size_t size);

private:
    void wait(GLsync* pSync);

    std::vector<BufferRangeLock> locks;
};

} // namespace sdbox

#endif