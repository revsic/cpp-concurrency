#ifndef LOCK_FREE_LIST_HPP
#define LOCK_FREE_LIST_HPP

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "../platform/constant.hpp"
#include "../platform/optional.hpp"

namespace LockFree {

    template <typename T>
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node() : data(), next(nullptr) {
            // Do Nothing
        }

        template <typename U>
        Node(U&& data) : data(std::forward<U>(data)), next(nullptr) {
            // Do Nothing
        }
    };

    template <typename T>
    class List {
    public:
        List() : head(), tail(&head), num_data(0) {
            // Do Nothing
        }

        ~List() {
            Node<T>* node = head.next;
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
            Node<T>* node = new Node<T>(data);

            Node<T>* prev = nullptr;
            do {
                prev = tail.load(std::memory_order_relaxed);
            } while (!tail.compare_exchange_weak(prev,
                                                 node,
                                                 std::memory_order_relaxed,
                                                 std::memory_order_relaxed));
            prev->next.store(node, std::memory_order_relaxed);
            ++num_data;
        }

        template <typename U = decltype(platform::prevent_deadlock)>
        T pop_front(U const& prevent_deadlock = platform::prevent_deadlock) {
            Node<T>* node;
            do {
                std::this_thread::sleep_for(prevent_deadlock);
                node = head.next.load(std::memory_order_relaxed);
            } while (
                !node
                || !head.next.compare_exchange_weak(node,
                                                    node->next,
                                                    std::memory_order_relaxed,
                                                    std::memory_order_relaxed));
            --num_data;
            T res = std::move(node->data);

            delete node;
            return res;
        }

        platform::optional<T> try_pop() {
            Node<T>* node = head.next.load(std::memory_order_relaxed);
            if (node
                && head.next.compare_exchange_weak(node,
                                                   node->next,
                                                   std::memory_order_relaxed,
                                                   std::memory_order_relaxed)) {
                --num_data;
                T res = std::move(node->data);

                delete node;
                return platform::optional<T>(std::move(res));
            }
            return platform::nullopt;
        }

        size_t size() const {
            return num_data;
        }

        Node<T>* node() {
            return head.next;
        }

    private:
        Node<T> head;
        std::atomic<Node<T>*> tail;

        std::atomic<size_t> num_data;
    };
}  // namespace LockFree

#endif