#ifndef CONTAINER_RING_BUFFER_HPP
#define CONTAINER_RING_BUFFER_HPP

#include <memory>
#include <type_traits>

template <typename T, typename = void>  // for stl compatiblity
class RingBuffer {
public:
    static_assert(std::is_default_constructible_v<T>,
                  "RingBuffer base type must be default constructible");

    RingBuffer() : RingBuffer(1) {
        // Do Nothing
    }

    RingBuffer(size_t size_buffer)
        : size_buffer(size_buffer), buffer(std::make_unique<T[]>(size_buffer)) {
        // Do Nothing
    }

    RingBuffer(RingBuffer const&) = delete;
    RingBuffer(RingBuffer&&) = delete;

    RingBuffer& operator=(RingBuffer const&) = delete;
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

    T const& front() const {
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

#endif