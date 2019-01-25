#include <catch2/catch.hpp>
#include <lockfree/channel.hpp>
#include <thread_pool.hpp>

#include <future>

TEST_CASE("Channel::Initializer", "[lockfree/channel]") {
    LockFree::Channel<int>();
    REQUIRE(true);
}

TEST_CASE("Channel::Add, Get", "[lockfree/channel]") {
    LockFree::Channel<int> channel;
    for (int i = 0; i < 10; ++i) {
        channel.Add(i);
    }

    for (int i = 0; i < 10; ++i) {
        platform::optional<int> out = channel.Get();
        REQUIRE(out.has_value());
        REQUIRE(out.value() == i);
    }
}

TEST_CASE("Channel::Add, TryGet", "[lockfree/channel]") {
    LockFree::Channel<int> channel;
    REQUIRE(channel.TryGet() == platform::nullopt);

    for (int i = 0; i < 10; ++i) {
        channel.Add(i);
    }

    for (int i = 0; i < 10; ++i) {
        platform::optional<int> out = channel.TryGet();
        REQUIRE(out.has_value());
        REQUIRE(out.value() == i);
    }
    REQUIRE(channel.TryGet() == platform::nullopt);
}

TEST_CASE("Channel::operator<<, >>", "[lockfree/channel]") {
    LockFree::Channel<int> channel;
    for (int i = 0; i < 10; ++i) {
        channel << i;
    }

    for (int i = 0; i < 10; ++i) {
        int out;
        channel >> out;
        REQUIRE(out == i);
    }

    for (int i = 0; i < 10; ++i) {
        channel << i;
    }

    for (int i = 0; i < 10; ++i) {
        platform::optional<int> out;
        channel >> out;

        REQUIRE(out.has_value());
        REQUIRE(out.value() == i);
    }
}

TEST_CASE("Channel::Close, Runnable, Readable", "[lockfree/channel]") {
    LockFree::Channel<int> channel;
    REQUIRE(channel.Runnable());
    REQUIRE(channel.Readable());

    channel.Add(1);
    REQUIRE(channel.Readable());

    channel.Get();
    REQUIRE(channel.Readable());

    channel.Add(1);
    channel.Close();

    REQUIRE(!channel.Runnable());
    REQUIRE(channel.Readable());

    LockFree::Channel<int> channel2;
    channel2.Close();

    REQUIRE(!channel2.Readable());
}

TEST_CASE("Channel::iterator", "[lockfree/channel]") {
    LockFree::Channel<int> channel;
    auto fut = std::async(std::launch::async, [&] {
        for (int i = 0; i < 10; ++i) {
            channel.Add(i);
        }
        channel.Close();
    });

    int acc = 0;
    for (int elem : channel) {
        acc += elem;
    }

    REQUIRE(acc == 45);
}

TEST_CASE("Concurrently Add and Get", "[lockfree/channel]") {
    LockFree::Channel<size_t> channel;
    UThreadPool<void> push_pool(5);
    UThreadPool<size_t> pop_pool(5);

    constexpr size_t test_num = 20;

    std::vector<std::future<void>> push_futs;
    auto push = push_pool.Add([&] {
        for (size_t i = 1; i <= test_num; ++i) {
            push_futs.push_back(push_pool.Add([&, i] { channel.Add(i); }));
        }
    });

    std::vector<std::future<size_t>> pop_futs;
    auto pop = pop_pool.Add([&] {
        for (size_t i = 0; i < test_num; ++i) {
            pop_futs.push_back(
                pop_pool.Add([&] { return channel.Get().value(); }));
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