#include "concurrency.hpp"

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
    if (fs::is_regular_file(path)) {
        return fs::file_size(path);
    }
    if (!fs::is_directory(path)) {
        return 0;
    }

    ThreadPool<ull> pool;
    UChannel<fs::path> channel;

    WaitGroup wg;
    std::vector<std::future<ull>> fut_vec;
    for (size_t i = 0; i < pool.GetNumThreads(); ++i) {
        fut_vec.emplace_back(pool.Add([&]{
            ull size = 0;
            for (auto const& path : channel) {
                for (auto const& dir : fs::directory_iterator(path)) {
                    if (dir.is_regular_file()) {
                        size += dir.file_size();
                    }
                    else if (dir.is_directory()) {
                        wg.Add();
                        channel.Add(dir.path());
                    }
                }
                wg.Done();
            }
            return size;
        }));
    }

    wg.Add();
    channel.Add(path);

    wg.Wait();
    channel.Close();

    ull size = 0;
    for (auto& f : fut_vec) {
        size += f.get();
    }
    return size;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: ./concurrency_example [DIR_PATH]" << std::endl;
        return 1;
    }
    std::string given = argv[1];

    fs::path path(given);
    if (!fs::exists(path)) {
        std::cout << "invalid file path" << std::endl;
        return 1;
    }

    auto unpar_start = chrono::steady_clock::now();
    ull unpar_size = sizeof_dir(path);
    auto unpar_end = chrono::steady_clock::now();

    auto unpar_dur = chrono::duration_cast<chrono::nanoseconds>(unpar_end - unpar_start).count();
    std::cout << "size: " << unpar_size << " / time: " << unpar_dur << "ns" << std::endl;

    auto par_start = chrono::steady_clock::now();
    ull par_size = par_sizeof_dir(path);
    auto par_end = chrono::steady_clock::now();

    auto par_dur = chrono::duration_cast<chrono::nanoseconds>(par_end - par_start).count();
    std::cout << "size: " << par_size << " / time: " << par_dur << "ns" << std::endl;

    return 0;
}