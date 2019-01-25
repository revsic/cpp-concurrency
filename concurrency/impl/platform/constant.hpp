#ifndef PLATFORM_CONSTANT_HPP
#define PLATFORM_CONSTANT_HPP

// merge:include
#include <chrono>
// merge:end

namespace platform {
#ifndef __APPLE__
    constexpr std::chrono::microseconds prevent_deadlock(5);
#else
    constexpr std::chrono::microseconds prevent_deadlock(150);
#endif
}  // namespace platform

#endif