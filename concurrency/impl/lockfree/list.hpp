#ifndef LOCK_FREE_LIST_HPP
#define LOCK_FREE_LIST_HPP

#include "../platform/optional.hpp"

#include <atomic>
#include <memory>

namespace LockFree {

    template <typename T>
    struct Node {
        T data;
        std::atomic<Node*> next;
        
        Node() : data(), next(nullptr) {
            // Do Nothing
        }

        template <typename U>
        Node(U&& data) : 
            data(std::forward<U>(data)), next(nullptr)
        {
            // Do Nothing
        }
    };

    template <typename T>
    class LockFreeList {
    public:
        LockFreeList() : head(), tail(&head) {
            // Do Nothing
        }

        ~LockFreeList() {
            Node<T>* data = head->next;
            while (data != nullptr) {
                Node<T>* next = data->next;
                delete data;
                data = next;
            }
        }

        LockFreeList(LockFreeList const&) = delete;
        LockFreeList(LockFreeList&&) = delete;

        LockFreeList& operator=(LockFreeList const&) = delete;
        LockFreeList& operator=(LockFreeList&&) = delete;

        void push_back(T const& data) {
            Node<T>* node = new Node<T>(data);
            
            Node<T>* prev = nullptr;
            do {
                prev = tail.load(std::memory_order_relaxed);
            } while (!tail.compare_exchange_weak(prev, node,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
            prev->next.store(node, std::memory_order_release);
        }

        T pop_front() {
            Node<T>* node;
            do {
                node = head.next.load(std::memory_order_acquire);
            } while (!node || !head.next.compare_exchange_weak(node, node->next,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed));
            T res = std::move(node->data);
            delete node;

            return res;
        }

        platform::optional<T> try_pop() {
            Node<T>* node = head.next.load(std::memory_order_relaxed);
            if (node && head.next.compare_exchange_weak(node, node->next,
                                                        std::memory_order_release,
                                                        std::memory_order_relaxed))
            {
                T res = std::move(node->data);
                delete node;

                return platform::optional<T>(std::move(res));
            }
            return platform::nullopt;
        }

    private:
        Node<T> head;
        std::atomic<Node<T>*> tail;
    };
}

#endif