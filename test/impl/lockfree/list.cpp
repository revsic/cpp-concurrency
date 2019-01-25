#include <catch2/catch.hpp>
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
    LockFree::Node<size_t>* node = list.head();
    while (node != nullptr) {
        acc += node->data;
        node = node->next;
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
}

TEST_CASE("List::pop_front", "[lockfree/list]") {
    LockFree::List<size_t> list;

    constexpr size_t test_num = 1000;
    for (size_t i = 1; i <= test_num; ++i) {
        list.push_back(i);
    }

    REQUIRE(list.size() == test_num);

    UThreadPool<size_t> pool;
    std::vector<std::future<size_t>> futs;
    for (size_t i = 0; i < test_num; ++i) {
        futs.emplace_back(pool.Add([&] { return list.pop_front().value(); }));
    }

    size_t acc = 0;
    for (auto& fut : futs) {
        acc += fut.get();
    }

    REQUIRE(acc == test_num * (test_num + 1) / 2);
    REQUIRE(list.head() == list.tail());
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
                pop_pool.Add([&] { return list.pop_front().value(); }));
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
    REQUIRE(list.head() == list.tail());
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
    REQUIRE(list.head() == list.tail());
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
                    std::this_thread::sleep_for(platform::prevent_deadlock);
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
    REQUIRE(list.head() == list.tail());
}

TEST_CASE("List::runnable, readable, interrupt, resume", "[lockfree/list]") {
    LockFree::List<int> list;
    REQUIRE(list.runnable());

    list.interrupt();
    list.push_back(-1);

    REQUIRE(!list.runnable());
    REQUIRE(!list.readable());
    REQUIRE(list.size() == 0);

    list.resume();
    list.push_back(-1);

    REQUIRE(list.runnable());
    REQUIRE(list.readable());
    REQUIRE(list.size() == 1);

    list.interrupt();
    auto res = list.pop_front();

    REQUIRE(!list.readable());
    REQUIRE(list.size() == 0);
    REQUIRE(res.has_value());
    REQUIRE(res.value() == -1);

    list.resume();
    list.push_back(-1);
    res = list.pop_front();

    REQUIRE(list.readable());
    REQUIRE(list.size() == 0);
    REQUIRE(res.has_value());
    REQUIRE(res.value() == -1);

    list.interrupt();
    REQUIRE(!list.readable());
}