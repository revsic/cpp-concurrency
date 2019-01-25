#ifndef PLATFORM_CONSTANT_HPP
#define PLATFORM_CONSTANT_HPP

// merge:include
#include <chrono>
// merge:end

namespace platform {
    using namespace std::literals;
#ifndef __APPLE__
    constexpr auto prevent_deadlock = 5us;
#else
    constexpr auto prevent_deadlock = 300us;
#endif
}  // namespace platform

#endif