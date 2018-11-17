#include "../concurrency.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace chrono = std::chrono;

ull sizeof_dir(const fs::path& path) {
    if (fs::is_regular_file(path)) {
        return fs::file_size(path);
    }

    ull size = 0;
    if (fs::is_directory(path)) {
        for (auto& dir : fs::directory_iterator(path)) {
            size += sizeof_dir(dir.path());
        }
    }

    return size;
}

ull par_sizeof_dir(const fs::path& path) {
    WaitGroup wg = 1;
    UChannel<ull> channel;
    UThreadPool<void> pool;

    std::function<void(fs::path const&)> par = [&](fs::path const& path) {
        if (fs::is_regular_file(path)) {
            channel << fs::file_size(path);
            return;
        }
        if (fs::is_directory(path)) {
            ull res = 0;
            for (auto const& dir : fs::directory_iterator(path)) {
                if (dir.is_regular_file()) {
                    res += dir.file_size();
                }
                else if (dir.is_directory()) {
                    wg.Add();
                    pool.Add([&, path=dir.path()]{ par(path); });
                }
            }
            channel << res;
            if (wg.Done() == 0) {
                channel.Close();
            }
        }
    };
    par(path);

    ull res = 0;
    for (auto const& size : channel) {
        res += size;
    }
    return res;
}

template <typename T, typename F, typename... Args>
auto perf(F&& func, Args&&... args) {
    auto start = chrono::steady_clock::now();
    auto res = func(std::forward<Args>(args)...);
    auto end = chrono::steady_clock::now();

    return std::make_tuple(res, chrono::duration_cast<T>(end - start));
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: ./concurrency_example [DIR_PATH]" << std::endl;
        return 1;
    }
    std::string given = argv[1];

    fs::path path(given);
    if (!fs::exists(path) || !fs::is_directory(path)) {
        std::cout << "invalid directory path" << std::endl;
        return 1;
    }

    auto list = { sizeof_dir, par_sizeof_dir };
    auto log = [](auto&& tup){
        auto[res, dur] = std::move(tup);
        std::cout << "size: " << res << " / time: " << dur.count() << "ns\n";
    };

    for (auto const& f : list) {
        log(perf<chrono::nanoseconds>(f, path));
    }

    return 0;
}