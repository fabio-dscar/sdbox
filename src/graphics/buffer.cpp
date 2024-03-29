#include <buffer.h>

using namespace sdbox;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace {
const GLenum OGLBufferTarget[] = {GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_UNIFORM_BUFFER};

constexpr nanoseconds FenceTimeout = 33ms;
}

Buffer::Buffer(BufferType type, std::size_t size, BufferFlag flags, const void* data) {
    create(type, size, flags, data);
}

Buffer::~Buffer() {
    if (handle != 0) {
        if (HasFlag(flags, BufferFlag::Persistent))
            glUnmapNamedBuffer(handle);

        glDeleteBuffers(1, &handle);
    }
}

void Buffer::create(BufferType type, std::size_t pSize, BufferFlag pFlags, const void* data) {
    target = OGLBufferTarget[static_cast<unsigned int>(type)];
    flags  = pFlags;
    size   = pSize;
    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, size, data, static_cast<GLbitfield>(flags));

    if (HasFlag(flags, BufferFlag::Persistent))
        ptr = reinterpret_cast<std::byte*>(
            glMapNamedBufferRange(handle, 0, size, static_cast<GLbitfield>(flags)));
}

void Buffer::bindRange(unsigned int index, std::size_t offset, std::size_t bSize) const {
    glBindBufferRange(target, index, handle, offset, bSize);
}

void SyncedBuffer::wait(GLsync* pSync) {
    GLenum res = glClientWaitSync(*pSync, 0, FenceTimeout.count());
    if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED)
        glDeleteSync(*pSync);
    else
        LOGD("Fence timeout expired.");
}

void SyncedBuffer::waitRange(std::size_t start, std::size_t pSize) {
    std::vector<BufferRangeLock> swapLocks;
    for (auto it = locks.begin(); it != locks.end(); ++it) {
        if (it->overlaps(start, pSize)) {
            wait(&it->lock);
        } else {
            swapLocks.push_back(*it);
        }
    }
    locks.swap(swapLocks);
}

void SyncedBuffer::lockRange(std::size_t start, std::size_t pSize) {
    GLsync syncName = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    locks.emplace_back(syncName, start, pSize);
}