#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <atomic>
#include <condition_variable>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>

#define CHANNEL_HPP
#define THREAD_POOL_HPP
#define WAIT_GROUP_HPP


template <typename T,
          typename = std::enable_if_t<std::is_default_constructible_v<T>>>
class RingBuffer {
public:
    RingBuffer() : RingBuffer(1) {
        // Do Nothing
    }

    RingBuffer(size_t size_buffer) :
        size_buffer(size_buffer), buffer(std::make_unique<T[]>(size_buffer))
    {
        // Do Nothing
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;

    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    template <typename... U>
    void emplace_back(U&&... args) {
        buffer[ptr_tail] = T(std::forward<U>(args)...);

        num_data += 1;
        ptr_tail = (ptr_tail + 1) % size_buffer;
    }

    void pop_front() {
        num_data -= 1;
        ptr_head = (ptr_head + 1) % size_buffer;
    }

    T& front() {
        return buffer[ptr_head];
    }

    const T& front() const {
        return buffer[ptr_head];
    }

    size_t size() const {
        return num_data;
    }

    size_t max_size() const {
        return size_buffer;
    }

private:
    size_t size_buffer;
    std::unique_ptr<T[]> buffer;

    size_t num_data = 0;
    size_t ptr_head = 0;
    size_t ptr_tail = 0;
};

template <typename T,
          typename Container = RingBuffer<T>> // or Container = std::list<T>
class Channel {
public:
    template <typename... U>
    Channel(U&&... args) : buffer(std::forward<U>(args)...) {
        // Do Nothing
    }

    Channel(const Channel&) = delete;
    Channel(Channel&&) = delete;

    Channel& operator=(const Channel&) = delete;
    Channel& operator=(Channel&&) = delete;

    template <typename... U>
    void Add(U&&... task) {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&]{ return !runnable || buffer.size() < buffer.max_size(); });

        if (runnable) {
            buffer.emplace_back(std::forward<U>(task)...);
        }
        cv.notify_all();
    }

    template <typename U>
    Channel& operator<<(U&& task) {
        Add(std::forward<U>(task));
        return *this;
    }

    std::optional<T> Get() {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&]{ return !runnable || buffer.size() > 0; });

        if (!runnable && buffer.size() == 0) return std::nullopt;

        T given = std::move(buffer.front());
        buffer.pop_front();

        cv.notify_all();
        return std::optional<T>(std::move(given));
    }

    std::optional<T> TryGet() {
        std::unique_lock lock(mtx, std::try_to_lock);
        if (lock.owns_lock() && buffer.size() > 0) {
            T given = std::move(buffer.front());
            buffer.pop_front();

            cv.notify_all();
            return std::optional<T>(std::move(given));
        }
        return std::nullopt;
    }

    Channel& operator>>(std::optional<T>& get) {
        get = Get();
        return *this;
    }

    Channel& operator>>(T& get) {
        std::optional<T> res = Get();
        if (res.has_value()) {
            get = std::move(res.value());
        }
        return *this;
    }

    void Close() {
        runnable = false;
        cv.notify_all();
    } 

    bool Runnable() const {
        return runnable;
    }

    bool Readable() const {
        return runnable || buffer.size() > 0;
    }

    struct Iterator {
        Channel& channel;
        std::optional<T> item;

        Iterator(Channel& channel, std::optional<T>&& item) : 
            channel(channel), item(std::move(item)) 
        {
            // Do Nothing
        }

        T& operator*() {
            return item.value();
        }

        const T& operator*() const {
            return item.value();
        }

        Iterator& operator++() {
            item = channel.Get();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return item != other.item;
        }
    };

    Iterator begin() {
        return Iterator(*this, Get());
    }

    Iterator end() {
        return Iterator(*this, std::nullopt);
    }

private:
    Container buffer;
    bool runnable = true;

    std::mutex mtx;
    std::condition_variable cv;
};

template <typename T>
using UChannel = Channel<T, std::list<T>>;

template <typename T, typename F>
struct Selectable {
    T& channel;
    F action;

    template <typename Fs>
    Selectable(T& channel, Fs&& action) :
        channel(channel), action(std::forward<Fs>(action))
    {
        // Do Nothing
    }
};

struct DefaultSelectable {
    bool Readable() const {
        return false;
    }

    std::optional<void*> TryGet() const {
        return { nullptr };
    }

    static DefaultSelectable channel;
};
DefaultSelectable DefaultSelectable::channel;

template <typename T>
struct case_m {
    T& channel;

    case_m(T& channel) : channel(channel) {
        // Do Nothing
    }

    template <typename F>
    Selectable<T, F> operator>>(F&& action) {
        return Selectable<T, F>(channel, std::forward<F>(action));
    }
};

inline case_m default_m(DefaultSelectable::channel);

template <typename F, typename V>
auto select_invoke(F&& action, V&& value) {
    if constexpr (std::is_invocable_v<F, V>) {
        return action(value);
    }
    else if constexpr (std::is_invocable_v<F>) {
        return action();
    }
}

template <typename... T>
void select(T&&... matches) {
    auto readable = [](auto&... selectable) {
        return (selectable.channel.Readable() || ...);
    };

    auto try_action = [](auto& match) {
        auto opt = match.channel.TryGet();
        if (opt.has_value()) {
            select_invoke(match.action, opt.value());
        }
    };

    while (readable(matches...)) {
        (try_action(matches), ...);
    }
}


template <typename T,
          typename Container = RingBuffer<std::packaged_task<T()>>>
class ThreadPool {
public:
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {
        // Do Nothing
    }

    template <typename... Args>
    ThreadPool(size_t num_threads, Args&&... args) :
        num_threads(num_threads),
        threads(std::make_unique<std::thread[]>(num_threads)),
        channel(std::forward<Args>(args)...)
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

private:
    bool runnable = true;
    size_t num_threads;
    std::unique_ptr<std::thread[]> threads;
    Channel<std::packaged_task<T()>, Container> channel;
};

template <typename T>
using UThreadPool = ThreadPool<T, std::list<std::packaged_task<T()>>>;


using ull = unsigned long long;

class WaitGroup {
public:
    WaitGroup() : visit(0) {
        // Do Nothing
    }

    WaitGroup(ull visit) : visit(visit) {
        // Do Nothing
    }

    ull Add() {
        return (visit += 1);
    }

    ull Done() {
        return (visit -= 1);
    }

    void Wait() {
        while (visit > 0) {
            std::this_thread::yield();
        }
    }

private:
    std::atomic<ull> visit;
};


#endif
