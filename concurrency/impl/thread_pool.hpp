#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <future>

#include "channel.hpp"
#include "lockfree/channel.hpp"

template <typename T, template <typename> class ChannelType>
using ChannelTypeGen = ChannelType<std::packaged_task<T()>>;

template <typename T, template <typename, template <typename, typename> class> class ChannelType,  template <typename, typename> class Container>
using ChannelTypeGen2 = ChannelType<std::packaged_task<T()>, Container>;

template <typename T, typename ChannelType = ChannelTypeGen2<T, Channel, RingBuffer>>
class ThreadPool {
public:
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {
        // Do Nothing
    }

    template <typename... Args>
    ThreadPool(size_t num_threads, Args&&... args)
        : runnable(true),
          num_threads(num_threads),
          channel(std::forward<Args>(args)...),
          threads(std::make_unique<std::thread[]>(num_threads)) {
        for (size_t i = 0; i < num_threads; ++i) {
            threads[i] = std::thread([this] {
                while (runnable) {
                    auto given = channel.Get();
                    if (!given.has_value()) {
                        break;
                    }
                    given.value()();
                }
            });
        }
    }

    ~ThreadPool() {
        Stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template <typename F>
    std::future<T> Add(F&& task) {
        std::packaged_task<T()> ptask(std::forward<F>(task));
        std::future<T> fut = ptask.get_future();
        channel.Add(std::move(ptask));
        return fut;
    }

    size_t GetNumThreads() const {
        return num_threads;
    }

    void Stop() {
        if (threads != nullptr) {
            runnable = false;
            channel.Close();

            for (size_t i = 0; i < num_threads; ++i) {
                if (threads[i].joinable()) {
                    threads[i].join();
                }
            }
            threads.reset();
        }
    }

private:
    bool runnable;
    size_t num_threads;

    ChannelType channel;
    std::unique_ptr<std::thread[]> threads;
};

template <typename T>
using UThreadPool = ThreadPool<T, ChannelTypeGen2<T, Channel, std::list>>;

template <typename T>
using LThreadPool = ThreadPool<T, ChannelTypeGen<T, LockFree::Channel>>;

#endif