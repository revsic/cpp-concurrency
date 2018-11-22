#ifndef SELECT_HPP
#define SELECT_HPP

#include "channel.hpp"

template <typename T, typename F>
struct Selectable {
    T& channel;
    F action;

    template <typename Fs>
    Selectable(T& channel, Fs&& action) :
        channel(channel), action(std::forward<Fs>(action))
    {
        // Do Nothing
    }
};

struct DefaultSelectable {
    bool Readable() const {
        return true;
    }

    std::optional<void*> TryGet() const {
        return { nullptr };
    }

    static DefaultSelectable channel;
};
DefaultSelectable DefaultSelectable::channel;

template <typename T>
struct case_m {
    T& channel;
    
    case_m(T& channel) : channel(channel) {
        // Do Nothing
    }

    template <typename F>
    Selectable<T, F> operator>>(F&& action) {
        return Selectable<T, F>(channel, std::forward<F>(action));
    }
};

inline case_m default_m(DefaultSelectable::channel);

template <typename A, typename V>
auto select_invoke(A&& action, V&& value) {
    if constexpr (std::is_invocable_v<A, V>) {
        return action(value);
    }
    else if constexpr (std::is_invocable_v<A>) {
        return action();
    }
    return;
}

template <typename... T>
void select(T&&... matches) {
    auto readable = [](auto&... selectable) {
        return (selectable.channel.Readable() || ...);
    };

    bool run = true;;
    auto try_action = [&](auto& match) {
        if (run) {
            auto opt = match.channel.TryGet();
            if (opt.has_value()) {
                run = false;
                select_invoke(match.action, opt.value());
            }
        }
    };

    while (!readable(matches...));
    (try_action(matches), ...);
}

#endif