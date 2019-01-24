#include <catch2/catch.hpp>
#include <iostream>
#include <lockfree/list.hpp>
#include <thread_pool.hpp>

TEST_CASE("List::Initializer", "[lockfree/list]") {
    LockFree::List<int>();
    REQUIRE(true);
}

TEST_CASE("List::push_back", "[lockfree/list]") {
    LockFree::List<size_t> list;
    UThreadPool<void> pool;

    constexpr size_t test_num = 1000;

    std::vector<std::future<void>> futs;
    for (size_t i = 1; i <= test_num; ++i) {
        futs.emplace_back(pool.Add([&, i] { list.push_back(i); }));
    }

    for (auto& fut : futs) {
        fut.get();
    }

    REQUIRE(list.size() == test_num);

    size_t acc = 0;
    LockFree::Node<size_t>* node = list.node();
    while (node != nullptr) {
        acc += node->data;
        node = node->next;
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
}

TEST_CASE("List::pop_back", "[lockfree/list]") {
    LockFree::List<size_t> list;

    constexpr size_t test_num = 1000;
    for (size_t i = 1; i <= test_num; ++i) {
        list.push_back(i);
    }

    REQUIRE(list.size() == test_num);

    UThreadPool<size_t> pool;
    std::vector<std::future<size_t>> futs;
    for (size_t i = 0; i < test_num; ++i) {
        futs.emplace_back(pool.Add([&] { return list.pop_front(); }));
    }

    size_t acc = 0;
    for (auto& fut : futs) {
        acc += fut.get();
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
}

TEST_CASE("Concurrently push and pop", "[lockfree/list]") {
    LockFree::List<size_t> list;
    UThreadPool<void> push_pool(5);
    UThreadPool<size_t> pop_pool(5);

    constexpr size_t test_num = 20;

    std::vector<std::future<void>> push_futs;
    auto push = push_pool.Add([&] {
        for (size_t i = 1; i <= test_num; ++i) {
            push_futs.push_back(push_pool.Add([&, i] { list.push_back(i); }));
        }
    });

    std::vector<std::future<size_t>> pop_futs;
    auto pop = pop_pool.Add([&] {
        for (size_t i = 0; i < test_num; ++i) {
            pop_futs.push_back(
                pop_pool.Add([&, i] { return list.pop_front(); }));
        }
        return 0;
    });

    push.wait();
    pop.wait();

    for (auto& fut : push_futs) {
        fut.wait();
    }

    size_t acc = 0;
    for (auto& fut : pop_futs) {
        acc += fut.get();
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
}

TEST_CASE("List::try_pop", "[lockfree/list]") {
    LockFree::List<size_t> list;

    constexpr size_t test_num = 1000;
    for (size_t i = 1; i <= test_num; ++i) {
        list.push_back(i);
    }

    REQUIRE(list.size() == test_num);

    UThreadPool<size_t> pool;
    std::vector<std::future<size_t>> futs;
    for (size_t i = 0; i < test_num; ++i) {
        futs.emplace_back(pool.Add([&] {
            platform::optional<size_t> res;
            do {
                res = list.try_pop();
            } while (!res.has_value());
            return res.value();
        }));
    }

    size_t acc = 0;
    for (auto& fut : futs) {
        acc += fut.get();
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
}

TEST_CASE("Concurrently push and try_pop", "[lockfree/list]") {
    using namespace std::literals;

    LockFree::List<size_t> list;
    UThreadPool<void> push_pool(5);
    UThreadPool<size_t> pop_pool(5);

    constexpr size_t test_num = 20;

    std::vector<std::future<void>> push_futs;
    auto push = push_pool.Add([&] {
        for (size_t i = 1; i <= test_num; ++i) {
            push_futs.push_back(push_pool.Add([&, i] { list.push_back(i); }));
        }
    });

    std::vector<std::future<size_t>> pop_futs;
    auto pop = pop_pool.Add([&] {
        for (size_t i = 0; i < test_num; ++i) {
            pop_futs.push_back(pop_pool.Add([&, i] {
                platform::optional<size_t> res;
                do {
                    std::this_thread::sleep_for(5us);
                    res = list.try_pop();
                } while (!res.has_value());
                return res.value();
            }));
        }
        return 0;
    });

    push.wait();
    pop.wait();

    for (auto& fut : push_futs) {
        fut.wait();
    }

    size_t acc = 0;
    for (auto& fut : pop_futs) {
        acc += fut.get();
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
}
