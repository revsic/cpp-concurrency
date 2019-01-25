#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <atomic>
#include <condition_variable>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

#define CHANNEL_ITER_HPP
#define RING_BUFFER_HPP
#define CHANNEL_HPP
#define LOCK_FREE_LIST_HPP
#define LOCK_FREE_CHANNEL_HPP
#define SELECT_HPP
#define THREAD_POOL_HPP
#define WAIT_GROUP_HPP

#include <chrono>

#ifndef __APPLE__
#include <optional>
#else
#include <experimental/optional>
#endif


namespace platform {
    using namespace std::literals;
#ifndef __APPLE__
    constexpr auto prevent_deadlock = 5us;
#else
    // constexpr auto prevent_deadlock = 150us;  // for personal mac
    constexpr auto prevent_deadlock = 500us;  // for azure-pipeline mac
#endif
}  // namespace platform


namespace platform {
#ifndef __APPLE__
    using nullopt_t = std::nullopt_t;
    constexpr auto nullopt = std::nullopt;

    template <typename T>
    using optional = std::optional<T>;

#else
    using nullopt_t = std::experimental::nullopt_t;
    constexpr auto nullopt = std::experimental::nullopt;

    template <typename T>
    class optional : public std::experimental::optional<T> {
    private:
        using super = std::experimental::optional<T>;

    public:
        constexpr optional() noexcept : super() {
            // Do Nothing
        }

        constexpr optional(nullopt_t) noexcept : super(nullopt) {
            // Do Nothing
        }

        constexpr optional(optional const& other) noexcept : super(other) {
            // Do Nothing
        }

        constexpr optional(optional&& other) noexcept
            : super(std::move(other)) {
            // Do Nothing
        }

        template <typename U>
        optional(optional<U> const& other) : super(other) {
            // Do Nothing
        }

        template <typename U>
        optional(optional<U>&& other) : super(std::move(other)) {
            // Do Noting
        }

        template <typename... Args>
        constexpr explicit optional(std::in_place_t, Args&&... args)
            : super(std::in_place, std::forward<Args>(args)...) {
            // Do Nothing
        }

        template <typename U, typename... Args>
        constexpr explicit optional(std::in_place_t,
                                    std::initializer_list<U> ilist,
                                    Args&&... args)
            : super(std::in_place,
                    std::move(ilist),
                    std::forward<Args>(args)...) {
            // Do Nothing
        }

        template <typename U = typename super::value_type>
        constexpr optional(U&& value) : super(std::forward<U>(value)) {
            // Do Nothing
        }

        optional& operator=(nullopt_t) noexcept {
            super::operator=(nullopt);
            return *this;
        }

        constexpr optional& operator=(optional const& other) {
            super::operator=(other);
            return *this;
        }

        constexpr optional& operator=(optional&& other) noexcept {
            super::operator=(std::move(other));
            return *this;
        }

        template <typename U = T>
        optional& operator=(U&& value) {
            super::operator=(std::forward<U>(value));
            return *this;
        }

        template <typename U>
        optional& operator=(optional<U> const& other) {
            super::operator=(other);
            return *this;
        }

        template <typename U>
        optional& operator=(optional<U>&& other) {
            super::operator=(std::move(other));
            return *this;
        }

        constexpr const T* operator->() const {
            return super::operator->();
        }

        constexpr T* operator->() {
            return super::operator->();
        }

        constexpr const T& operator*() const& {
            return super::operator*();
        }

        constexpr T& operator*() & {
            return super::operator*();
        }

        constexpr const T&& operator*() const&& {
            return std::move(super::operator*());
        }

        constexpr T&& operator*() && {
            return std::move(super::operator*());
        }

        constexpr operator bool() const noexcept {
            return static_cast<bool>(static_cast<super const&>(*this));
        }

        constexpr bool has_value() const noexcept {
            return static_cast<bool>(*this);
        }

        constexpr T& value() & {
            return **this;
        }

        constexpr T const& value() const& {
            return **this;
        }

        constexpr T&& value() && {
            return std::move(**this);
        }

        constexpr T const&& value() const&& {
            return std::move(**this);
        }
    };
#endif
}  // namespace platform


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
          template <typename Elem, typename = std::allocator<Elem>>
          class Container = RingBuffer>  // or Container = std::list
class Channel {
public:
    using value_type = T;
    using iterator = ChannelIterator<T, Channel>;

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

    iterator begin() {
        return iterator(*this, Get());
    }

    iterator end() {
        return iterator(*this, platform::nullopt);
    }

private:
    Container<T> buffer;
    std::atomic<bool> runnable = true;

    std::mutex mtx;
    std::condition_variable cv;
};

template <typename T>
using UChannel = Channel<T, std::list>;


namespace LockFree {

    template <typename T>
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node() : data(), next(nullptr) {
            // Do Nothing
        }

        template <typename... U>
        Node(U&&... data) : data(std::forward<U>(data)...), next(nullptr) {
            // Do Nothing
        }
    };

    template <typename T>
    class List {
    public:
        List() : m_head(nullptr), m_tail(nullptr), m_runnable(true), m_size(0) {
            // Do Nothing
        }

        ~List() {
            m_runnable.store(false, std::memory_order_release);

            Node<T>* node = m_head.load();
            while (node != nullptr) {
                Node<T>* next = node->next;
                delete node;
                node = next;
            }
        }

        List(List const&) = delete;
        List(List&&) = delete;

        List& operator=(List const&) = delete;
        List& operator=(List&&) = delete;

        void push_back(T const& data) {
            push_node(new Node<T>(data));
        }

        void push_back(T&& data) {
            push_node(new Node<T>(std::move(data)));
        }

        template <typename... U>
        void emplace_back(U&&... args) {
            push_node(new Node<T>(std::forward<U>(args)...));
        }

        void push_node(Node<T>* node) {
            bool run = false;
            Node<T>* prev = nullptr;
            do {
                run = runnable();
                prev = m_tail.load(std::memory_order_relaxed);
            } while (
                run
                && !m_tail.compare_exchange_weak(prev,
                                                 node,
                                                 std::memory_order_relaxed,
                                                 std::memory_order_relaxed));
            if (run) {
                if (prev != nullptr) {
                    prev->next.store(node, std::memory_order_relaxed);
                }
                else {
                    m_head.store(node, std::memory_order_relaxed);
                }
                ++m_size;
            }
        }

        template <typename U = decltype(platform::prevent_deadlock)>
        platform::optional<T> pop_front(U const& prevent_deadlock
                                        = platform::prevent_deadlock) {
            bool run = false;
            Node<T>* node = nullptr;
            do {
                std::this_thread::sleep_for(prevent_deadlock);

                run = readable();
                node = m_head.load(std::memory_order_relaxed);
            } while (run
                     && (!node
                         || !m_head.compare_exchange_weak(
                                node,
                                node->next,
                                std::memory_order_relaxed,
                                std::memory_order_relaxed)));
            if (run) {
                if (node->next == nullptr) {
                    m_tail.store(nullptr, std::memory_order_relaxed);
                }
                --m_size;
                T res = std::move(node->data);

                delete node;
                return platform::optional<T>(std::move(res));
            }
            return platform::nullopt;
        }

        platform::optional<T> try_pop() {
            Node<T>* node = m_head.load(std::memory_order_relaxed);
            if (readable() && node
                && m_head.compare_exchange_weak(node,
                                                node->next,
                                                std::memory_order_relaxed,
                                                std::memory_order_relaxed)) {
                if (node->next == nullptr) {
                    m_tail.store(nullptr, std::memory_order_relaxed);
                }
                --m_size;
                T res = std::move(node->data);

                delete node;
                return platform::optional<T>(std::move(res));
            }
            return platform::nullopt;
        }

        size_t size() const {
            return m_size.load(std::memory_order_relaxed);
        }

        Node<T>* head() {
            return m_head.load(std::memory_order_relaxed);
        }

        Node<T>* tail() {
            return m_tail.load(std::memory_order_relaxed);
        }

        bool runnable() const {
            return m_runnable.load(std::memory_order_relaxed);
        }

        bool readable() const {
            return runnable()
                   || m_head.load(std::memory_order_relaxed) != nullptr;
        }

        void interrupt() {
            m_runnable.store(false, std::memory_order_relaxed);
        }

        void resume() {
            m_runnable.store(true, std::memory_order_relaxed);
        }

    private:
        std::atomic<Node<T>*> m_head;
        std::atomic<Node<T>*> m_tail;

        std::atomic<bool> m_runnable;
        std::atomic<size_t> m_size;
    };
}  // namespace LockFree


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
            get = buffer.pop_front();
            return *this;
        }

        Channel& operator>>(T& get) {
            platform::optional<T> res = buffer.pop_front();
            if (res.has_value()) {
                get = std::move(res.value());
            }
            return *this;
        }

        void Close() {
            buffer.interrupt();
        }

        bool Runnable() const {
            return buffer.runnable();
        }

        bool Readable() const {
            return buffer.readable();
        }

        iterator begin() {
            return iterator(*this, Get());
        }

        iterator end() {
            return iterator(*this, platform::nullopt);
        }

    private:
        List<T> buffer;
    };
}  // namespace LockFree


template <typename T, typename F>
struct Selectable {
    T& channel;
    F action;

    template <typename Fs>
    Selectable(T& channel, Fs&& action)
        : channel(channel), action(std::forward<Fs>(action)) {
        // Do Nothing
    }
};

struct DefaultSelectable {
    bool Readable() const {
        return true;
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

template <typename A, typename V>
auto select_invoke(A&& action, V&& value) {
    if constexpr (std::is_invocable_v<A, V>) {
        return action(value);
    }
    else if constexpr (std::is_invocable_v<A>) {
        return action();
    }
    return;
}

template <typename... T>
void select(T&&... matches) {
    auto readable = [](auto&... selectable) {
        return (selectable.channel.Readable() || ...);
    };

    bool run = true;
    auto try_action = [&](auto& match) {
        if (run) {
            auto opt = match.channel.TryGet();
            if (opt.has_value()) {
                run = false;
                select_invoke(match.action, opt.value());
            }
        }
    };

    while (!readable(matches...))
        ;
    (try_action(matches), ...);
}


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
