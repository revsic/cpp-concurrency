#ifndef WAIT_GROUP_HPP
#define WAIT_GROUP_HPP

#include <atomic>
#include <thread>

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

    template <typename F>
    auto Wait(F&& func) {
        Wait();
        return func();
    }

private:
    std::atomic<ull> visit;
};

#endif