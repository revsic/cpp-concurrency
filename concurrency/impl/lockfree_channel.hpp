#ifndef LOCK_FREE_CHANNEL_HPP
#define LOCK_FREE_CHANNEL_HPP

#include <atomic>
#include <memory>

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
    LockFreeList() : node(std::make_unique<Node<T>>());

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

    Node<T> pop_front() {
        Node<T>* node;
        do {
            node = head.load(std::memory_order_relaxed);
            while (!node->next.load(std::memory_order_acquire)); // spin lock

        } while (!head.compare_exchange_weak(node, node->next,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));

        Node<T> res = *node;

        while (!node->set.load(std::memory_order_acquire));
        delete node;

        return res;
    }

    
private:
    std::atomic<Node<T>*> head;
    std::atomic<Node<T>*> tail;
};

class LockFreeChannel {
public:

private:

};

#endif