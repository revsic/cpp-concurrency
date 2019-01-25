#ifndef LOCK_FREE_CHANNEL_HPP
#define LOCK_FREE_CHANNEL_HPP

#include "../channel_iter.hpp"
#include "list.hpp"

namespace LockFree {
    template <typename T>
    class Channel {
    public:
        using value_type = T;
        using iterator = ChannelIterator<T, Channel>;

        Channel() : buffer() {
            // Do Nothing
        }

        Channel(Channel const&) = delete;
        Channel(Channel&&) = delete;

        Channel& operator=(Channel const&) = delete;
        Channel& operator=(Channel&&) = delete;

        template <typename... U>
        void Add(U&&... task) {
            buffer.emplace_back(std::forward<U>(task)...);
        }

        template <typename U>
        Channel& operator<<(U&& task) {
            buffer.emplace_back(std::forward<U>(task));
            return *this;
        }

        platform::optional<T> Get() {
            return buffer.pop_front();
        }

        platform::optional<T> TryGet() {
            return buffer.try_pop();
        }

        Channel& operator>>(platform::optional<T>& get) {
            get = buffer.try_pop();
            return *this;
        }

        Channel& operator>>(T& get) {
            platform::optional<T> res = buffer.pop_front();
            if (res.has_value()) {
                get = std::move(res.value());
            }
            return *this;
        }

        void Closs() {
            buffer.interrupt();
        }

        bool Runnable() const {
            return buffer.runnable();
        }

        bool Readable() {
            return buffer.readable();
        }

        iterator begin() {
            return ChannelIterator(*this, Get());
        }

        iterator end() {
            return ChannelIterator(*this, platform::nullopt);
        }

        private: 
            List<T> buffer;
    };
}  // namespace LockFree

#endif