#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "channel_iter.hpp"
#include "lockbase/thread_safe.hpp"
#include "platform/optional.hpp"

template <typename Container>
class Channel {
public:
    using value_type = typename Container::value_type;
    using iterator = ChannelIterator<value_type, Container>;

    template <typename... U>
    Channel(U&&... args) : buffer(std::forward<U>(args)...) {
        // Do Nothing
    }

    Channel(Channel const&) = delete;
    Channel(Channel&&) = delete;

    Channel& operator=(const Channel&) = delete;
    Channel& operator=(Channel&&) = delete;

    template <typename... U>
    void Add(U&&... args) {
        buffer.emplace_back(std::forward<U>(args)...);
    }

    template <typename U>
    Channel& operator<<(U&& task) {
        Add(std::forward<U>(task));
        return *this;
    }

    platform::optional<value_type> Get() {
        return buffer.pop_front();
    }

    platform::optional<value_type> TryGet() {
        return buffer.try_pop();
    }

    Channel& operator>>(platform::optional<value_type>& get) {
        get = Get();
        return *this;
    }

    Channel& operator>>(value_type& get) {
        platform::optional<value_type> res = Get();
        if (res.has_value()) {
            get = std::move(res.value());
        }
        return *this;
    }

    void Close() {
        buffer.close();
    }

    bool Runnable() const {
        return buffer.runnable();
    }

    bool Readable() {
        return buffer.Readable();
    }

    iterator begin() {
        return iterator(*this, Get());
    }

    iterator end() {
        return iterator(*this, platform::nullopt);
    }

private:
    Container buffer;
};

#endif