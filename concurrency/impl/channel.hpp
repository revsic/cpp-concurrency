#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "platform/optional.hpp"
#include "ring_buffer.hpp"

template <typename T,
          template <typename Elem, typename = std::allocator<Elem>>
          class Container = RingBuffer>  // or Container = std::list
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
        cv.wait(lock,
                [&] { return !runnable || buffer.size() < buffer.max_size(); });

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

    platform::optional<T> Get() {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&] { return !runnable || buffer.size() > 0; });

        if (!runnable && buffer.size() == 0) {
            return platform::nullopt;
        }

        T given = std::move(buffer.front());
        buffer.pop_front();

        cv.notify_all();
        return platform::optional<T>(std::move(given));
    }

    platform::optional<T> TryGet() {
        std::unique_lock lock(mtx, std::try_to_lock);
        if (lock.owns_lock() && buffer.size() > 0) {
            T given = std::move(buffer.front());
            buffer.pop_front();

            cv.notify_all();
            return platform::optional<T>(std::move(given));
        }
        return platform::nullopt;
    }

    Channel& operator>>(platform::optional<T>& get) {
        get = Get();
        return *this;
    }

    Channel& operator>>(T& get) {
        platform::optional<T> res = Get();
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

    bool Readable() {
        std::unique_lock lock(mtx);
        return runnable || buffer.size() > 0;
    }

    struct Iterator {
        Channel& channel;
        platform::optional<T> item;

        Iterator(Channel& channel, platform::optional<T>&& item)
            : channel(channel), item(std::move(item)) {
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
        return Iterator(*this, platform::nullopt);
    }

private:
    Container<T> buffer;
    std::atomic<bool> runnable = true;

    std::mutex mtx;
    std::condition_variable cv;
};

template <typename T>
using UChannel = Channel<T, std::list>;

#endif