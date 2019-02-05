#ifndef LOCKBASE_THREAD_SAFE_HPP
#define LOCKBASE_THREAD_SAFE_HPP

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

#include "../platform/optional.hpp"

template <typename Cont, typename Mutex = std::mutex>
class ThreadSafe {
public:
    using value_type = typename Cont::value_type;

    template <typename... Args>
    ThreadSafe(Args&&... args)
        : m_runnable(true), buffer(std::forward<Args>(args)...) {
        // Do Nothing
    }

    ~ThreadSafe() {
        close();
    }

    ThreadSafe(ThreadSafe const&) = delete;
    ThreadSafe(ThreadSafe&&) = delete;

    ThreadSafe& operator=(ThreadSafe const&) = delete;
    ThreadSafe& operator=(ThreadSafe&&) = delete;

    template <typename... U>
    void emplace_back(U&&... args) {
        std::unique_lock lock(mutex);
        cond.wait(lock, [&] {
            return !m_runnable || buffer.size() < buffer.max_size();
        });

        if (m_runnable) {
            buffer.emplace_back(std::forward<U>(args)...);
        }
        cond.notify_all();
    }

    void push_back(value_type const& value) {
        std::unique_lock lock(mutex);
        cond.wait(lock, [&] {
            return !m_runnable || buffer.size() < buffer.max_size();
        });

        if (m_runnable) {
            buffer.push_back(value);
        }
        cond.notify_all();
    }

    void push_back(value_type&& value) {
        std::unique_lock lock(mutex);
        cond.wait(lock, [&] {
            return !m_runnable || buffer.size() < buffer.max_size();
        });

        if (m_runnable) {
            buffer.push_back(std::move(value));
        }
        cond.notify_all();
    }

    platform::optional<value_type> pop_front() {
        std::unique_lock lock(mutex);
        cond.wait(lock, [&] { return !m_runnable || buffer.size() > 0; });

        if (!m_runnable && buffer.size() == 0) {
            return platform::nullopt;
        }

        value_type given = std::move(buffer.front());
        buffer.pop_front();

        cond.notify_all();
        return platform::optional<value_type>(std::move(given));
    }

    platform::optional<value_type> try_pop() {
        std::unique_lock lock(mutex, std::try_to_lock);
        if (lock.owns_lock() && buffer.size() > 0) {
            value_type given = std::move(buffer.front());
            buffer.pop_front();

            cond.notify_all();
            return platform::optional<value_type>(std::move(given));
        }
        return platform::nullopt;
    }

    void close() {
        m_runnable = false;
        cond.notify_all();
    }

    bool runnable() const {
        return m_runnable;
    }

    bool readable() {
        std::unique_lock lock(mutex);
        return m_runnable || buffer.size() > 0;
    }

private:
    bool m_runnable;
    Cont buffer;

    Mutex mutex;
    std::condition_variable cond;
};

template <typename T>
using TSList = ThreadSafe<std::list<T>>;

#endif