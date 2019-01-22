#include "../concurrency.hpp"
#include <iostream>
#include <chrono>

using namespace std::literals;

UThreadPool<void> global_pool;
inline auto sleep = [](auto dur) { std::this_thread::sleep_for(dur); };

template <typename T>
auto Tick(T dur, UThreadPool<void>& pool = global_pool) {
    auto tick = std::make_unique<UChannel<int>>();;
    pool.Add([tick = tick.get()]{
        while (tick->Runnable()) {
            sleep(100ms);
            tick->Add(0);
        }
    });
    return std::move(tick);
}

template <typename T>
auto After(T dur, UThreadPool<void>& pool = global_pool) {
    auto after = std::make_unique<UChannel<int>>();
    pool.Add([=, after = after.get()]{ sleep(dur); after->Add(0); });
    return std::move(after);
}

int main() {
    auto tick = Tick(100ms);
    auto boom = After(500ms);

    bool cont = true;
    while (cont) {
        select(
            case_m(*tick) >> []{ 
                std::cout << "tick." << std::endl; 
            },
            case_m(*boom) >> [&]{ 
                std::cout << "boom !" << std::endl;
                cont = false; 
            },
            default_m >> [&]{
                std::cout << "." << std::endl;
                sleep(50ms);
            }
        );
    }
    tick->Close();
}