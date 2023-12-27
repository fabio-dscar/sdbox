#ifndef __SDBOX_RINGBUFFER_H__
#define __SDBOX_RINGBUFFER_H__

#include <buffer.h>

namespace sdbox {

struct BufferBind {
    unsigned int bindIdx = 0;
    std::size_t  offset  = 0;
    std::size_t  size    = 0;
};

// Fence synchronized ring buffer with _num_ slots
class RingBuffer : private SyncedBuffer {
public:
    RingBuffer() = default;

    void create(
        BufferType type, unsigned int num, std::size_t size, BufferFlag flags = BufferFlag::None);

    template<typename T>
    T* get() const {
        return Buffer::get<T>(currIdx * baseSize);
    }

    template<typename T>
    T* getBind(unsigned int idx) const {
        DCHECK_LT(idx, binds.size());

        std::size_t offset = binds[idx].offset;
        return Buffer::get<T>(currIdx * baseSize + offset);
    }

    void lock();
    void swap();
    void lockAndSwap();
    void wait();

    void registerBind(unsigned int idx, std::size_t offset, std::size_t size);
    void rebind() const;

private:
    std::size_t  baseSize = 0;
    unsigned int numSlots = 0;
    unsigned int currIdx  = 0;

    std::vector<BufferBind> binds;
};

} // namespace sdbox

#endif