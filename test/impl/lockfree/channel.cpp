#include <catch2/catch.hpp>
#include <lockfree/channel.hpp>
#include <thread_pool.hpp>

TEST_CASE("Channel::Initializer", "[lockfree/channel]") {
    LockFree::Channel<int>();
    REQUIRE(true);
}