#ifndef PLATFORM_OPTIONAL_HPP
#define PLATFORM_OPTIONAL_HPP

// merge:np_include
#ifndef __APPLE__
#include <optional>
#else
#include <experimental/optional>
#endif
// merge:end

namespace platform {
#ifndef __APPLE__
    using nullopt_t = std::nullopt_t;
    constexpr auto nullopt = std::nullopt;

    template <typename T>
    using optional = std::optional<T>;

#else
    using nullopt_t = std::experimental::nullopt_t;
    constexpr auto nullopt = std::experimental::nullopt;

    template <typename T>
    class optional : public std::experimental::optional<T> {
    private:
        using super = std::experimental::optional<T>;

    public:
        constexpr optional() noexcept : super() {
            // Do Nothing
        }

        constexpr optional(nullopt_t) noexcept : super(nullopt) {
            // Do Nothing
        }

        constexpr optional(optional const& other) noexcept : super(other) {
            // Do Nothing
        }

        constexpr optional(optional&& other) noexcept
            : super(std::move(other)) {
            // Do Nothing
        }

        template <typename U>
        optional(optional<U> const& other) : super(other) {
            // Do Nothing
        }

        template <typename U>
        optional(optional<U>&& other) : super(std::move(other)) {
            // Do Noting
        }

        template <typename... Args>
        constexpr explicit optional(std::in_place_t, Args&&... args)
            : super(std::in_place, std::forward<Args>(args)...) {
            // Do Nothing
        }

        template <typename U, typename... Args>
        constexpr explicit optional(std::in_place_t,
                                    std::initializer_list<U> ilist,
                                    Args&&... args)
            : super(std::in_place,
                    std::move(ilist),
                    std::forward<Args>(args)...) {
            // Do Nothing
        }

        template <typename U = typename super::value_type>
        constexpr optional(U&& value) : super(std::forward<U>(value)) {
            // Do Nothing
        }

        optional& operator=(nullopt_t) noexcept {
            super::operator=(nullopt);
            return *this;
        }

        constexpr optional& operator=(optional const& other) {
            super::operator=(other);
            return *this;
        }

        constexpr optional& operator=(optional&& other) noexcept {
            super::operator=(std::move(other));
            return *this;
        }

        template <typename U = T>
        optional& operator=(U&& value) {
            super::operator=(std::forward<U>(value));
            return *this;
        }

        template <typename U>
        optional& operator=(optional<U> const& other) {
            super::operator=(other);
            return *this;
        }

        template <typename U>
        optional& operator=(optional<U>&& other) {
            super::operator=(std::move(other));
            return *this;
        }

        constexpr const T* operator->() const {
            return super::operator->();
        }

        constexpr T* operator->() {
            return super::operator->();
        }

        constexpr const T& operator*() const& {
            return super::operator*();
        }

        constexpr T& operator*() & {
            return super::operator*();
        }

        constexpr const T&& operator*() const&& {
            return std::move(super::operator*());
        }

        constexpr T&& operator*() && {
            return std::move(super::operator*());
        }

        constexpr operator bool() const noexcept {
            return static_cast<bool>(static_cast<super const&>(*this));
        }

        constexpr bool has_value() const noexcept {
            return static_cast<bool>(*this);
        }

        constexpr T& value() & {
            return **this;
        }

        constexpr T const& value() const& {
            return **this;
        }

        constexpr T&& value() && {
            return std::move(**this);
        }

        constexpr T const&& value() const&& {
            return std::move(**this);
        }
    };
#endif
}  // namespace platform

#endif