#ifndef LOCKFREE_LIST_HPP
#define LOCKFREE_LIST_HPP

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>

#include "../platform/constant.hpp"

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
        std::optional<T> pop_front(U const& prevent_deadlock
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
                return std::make_optional(std::move(res));
            }
            return std::nullopt;
        }

        std::optional<T> try_pop() {
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
                return std::make_optional(std::move(res));
            }
            return std::nullopt;
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

#endif