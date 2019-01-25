#ifndef CHANNEL_ITER_HPP
#define CHANNEL_ITER_HPP

#include "platform/optional.hpp"

template <typename T, typename Channel>
class ChannelIterator {
public:
    ChannelIterator(Channel& channel, platform::optional<T>&& item)
        : channel(channel), item(std::move(item)) {
        // Do Nothing
    }

    T& operator*() {
        return item.value();
    }

    const T& operator*() const {
        return item.value();
    }

    ChannelIterator& operator++() {
        item = channel.Get();
        return *this;
    }

    bool operator!=(const ChannelIterator& other) const {
        return item != other.item;
    }

private:
    Channel& channel;
    platform::optional<T> item;
};

#endif