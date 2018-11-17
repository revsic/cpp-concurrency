#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "channel.hpp"
#include <future>

template <typename T>
class ThreadPool {
public:
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {
        // Do Nothing
    }

    ThreadPool(size_t num_threads) : ThreadPool(num_threads, 1) {
        // Do Nothing
    }

    ThreadPool(size_t num_threads, size_t size_buffer) :
        num_threads(num_threads),
        threads(std::make_unique<std::thread[]>(num_threads)),
        channel(size_buffer)
    {
        for (size_t i = 0; i < num_threads; ++i) {
            threads[i] = std::thread([this]{ 
                while (runnable) {
                    auto given = channel.Get();
                    if (!given.has_value()) break;
                    given.value()();
                }
            });
        }
    }

    ~ThreadPool() {
        if (threads != nullptr) {
            runnable = false;
            channel.Close();

            for (size_t i = 0; i < num_threads; ++i) {
                if (threads[i].joinable()) {
                    threads[i].join();
                }
            }
        }
    }

    std::future<T> Add(std::function<T()>&& task) {
        std::packaged_task<T()> ptask(std::move(task));
        std::future<T> fut = ptask.get_future();
        channel.Add(std::move(ptask));
        return fut;
    }

    size_t GetNumThreads() const {
        return num_threads;
    }

private:
    bool runnable = true;
    size_t num_threads;
    std::unique_ptr<std::thread[]> threads;
    Channel<std::packaged_task<T()>> channel;
};

#endif