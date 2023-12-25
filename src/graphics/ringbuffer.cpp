#include <ringbuffer.h>

using namespace sdbox;

void RingBuffer::create(BufferType type, unsigned int num, std::size_t size, unsigned int flags) {
    numSlots = num;
    baseSize = size;
    Buffer::create(type, baseSize * numSlots, flags, NULL);
}

void RingBuffer::lock() {
    lockRange(currIdx * baseSize, baseSize);
}

void RingBuffer::swap() {
    currIdx = (currIdx + 1) % numSlots;
}

void RingBuffer::lockAndSwap() {
    lock();
    swap();
}

void RingBuffer::wait() {
    waitRange(currIdx * baseSize, baseSize);
}

void RingBuffer::registerBind(unsigned int idx, std::size_t offset, std::size_t size) {
    binds.emplace_back(idx, offset, size);
}

void RingBuffer::rebind() const {
    glBindBuffer(target, handle);
    for (const auto& bind : binds)
        bindRange(bind.bindIdx, bind.offset + currIdx * baseSize, bind.size);
    glBindBuffer(target, 0);
}