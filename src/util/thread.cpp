#include <thread.h>

using namespace sdbox;

namespace {
thread_local std::string ThreadName = "unnamed";
}

void sdbox::SetThreadName(const std::string& name) {
    ThreadName = name;
}

const std::string& sdbox::GetThreadName() {
    return ThreadName;
}

ThreadPool::ThreadPool(std::size_t numThreads, InitThreadFunc&& initFunc) {
    for (std::size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([&, i, initFunc] {
            if (initFunc)
                initFunc(i);

            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock lock{queueMutex};

                    condition.wait(lock, [&] { return !tasks.empty() || stop; });

                    if (tasks.empty() && stop)
                        return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                task();
            };
        });
    }
}

void ThreadPool::stopWorkers() {
    {
        std::lock_guard lock{queueMutex};
        if (stop)
            return; // Threads already stopped and joined

        stop = true;
    }

    condition.notify_all();
    for (auto& worker : workers)
        worker.join();
}