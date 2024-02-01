#ifndef SDBOX_THREAD_H
#define SDBOX_THREAD_H

#include <sdbox.h>

#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <future>

namespace sdbox {

void               SetThreadName(const std::string& name);
const std::string& GetThreadName();

using InitThreadFunc = std::function<void(std::size_t)>;

class ThreadPool {
public:
    explicit ThreadPool(std::size_t numThreads, InitThreadFunc&& initFunc = nullptr);
    ~ThreadPool() { stopWorkers(); }

    void stopWorkers();

    ThreadPool(ThreadPool&&)                 = delete;
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using returnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<returnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<returnType> result = task->get_future();

        {
            std::lock_guard lock(queueMutex);

            DCHECK(!stop);
            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();

        return result;
    }

private:
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread>          workers;
    std::mutex                        queueMutex;
    std::condition_variable           condition;
    bool                              stop = false;
};

} // namespace sdbox

#endif