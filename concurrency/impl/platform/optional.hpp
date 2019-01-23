#ifndef PLATFORM_OPTIONAL_HPP
#define PLATFORM_OPTIONAL_HPP

//merge:include
#ifndef __APPLE__
#include <optional>
#else
#include <experimental/optional>
#endif
//merge:end

namespace platform {
#ifndef __APPLE__
    constexpr auto nullopt = std::nullopt;

    template <typename T>
    using optional = std::optional<T>;

#else
    constexpr auto nullopt = std::experimental::nullopt;

    template <typename T>
    class optional : public std::experimental::optional<T> {
    public:
        constexpr T& value() & {
            return **this;
        }

        constexpr T&& value() && {
            return std::move(**this);
        }

        constexpr bool has_value() const noexcept {
            return static_cast<bool>(*this);
        }
    };
#endif
}

#endif