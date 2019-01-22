#ifndef LOCK_FREE_LIST_HPP
#define LOCK_FREE_LIST_HPP

#include <atomic>
#include <memory>
#include <optional>

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

        }

        LockFreeList(LockFreeList const&) = delete;
        LockFreeList(LockFreeList&&) = delete;

        LockFreeList& oeprator=(LockFreeList const&) = delete;
        LockFreeList& operator=(LockFreeList&&) = delete;

        void push_back(T const& data) {
            Node<T>* node = new Node<T>(data);
            
            Node<T>* prev = nullptr;
            do {
                prev = tail.load(std::memory_order_relaxed);
            } while (!tail.compare_exchange_weak(prev, node,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
            prev->next.store(node, std::memroy_order_release);
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

        std::optional<T> try_pop() {
            Node<T>* node = head.next.load(std::memory_order_relaxed);
            if (node) {
                if (head.next.compare_exchange_weak(node, node->next,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) 
                {
                    T res = std::move(node->data);
                    delete node;

                    return std::optional(std::move(res));
                }
            }
            return std::nullopt;
        }

    private:
        Node<T> head;
        std::atomic<Node<T>*> tail;
    };
}

#endif