# cpp-concurrency

[![License](https://img.shields.io/badge/Licence-MIT-blue.svg)](https://github.com/revsic/cpp-concurrency/blob/master/LICENSE)
[![Build Status](https://dev.azure.com/revsic99/cpp-concurrency/_apis/build/status/revsic.cpp-concurrency?branchName=master)](https://dev.azure.com/revsic99/cpp-concurrency/_build/latest?definitionId=2?branchName=master)

C++ implementation of golang style concurrency

## Usage

Use existing single header [concurrency.hpp](./concurrency.hpp) or run [script](./script/merge.py) to merge multiple headers in directory [concurrency](./concurrency).
```
python -m script.merge
```

Include [concurrency.hpp](./concurrency.hpp) to use it.
```C++
#include "concurrency.hpp"

ThreadPool<int> pool;
LChannel<int> channel;
WaitGroup wg;
```

## Sample

Send tick and bomb
```
g++ -o tick ./sample/tick.cpp -std=c++17 -lpthread #linux
cl /EHsc /std:c++17 ./sample/tick.cpp              #windows
```

Calculate directory size concurrently.
```
g++ -o dir_size ./sample/dir_size.cpp -std=c++17 -lstdc++fs -lpthread
cl /EHsc /std:c++17 ./sample/dir_size.cpp
```

## Channel

- RChannel<T> : finite capacity channel, if capacity exhausted, block channel and wait for space.
- LChannel<T> : list like channel.

Add and get from channel.
```C++
RChannel<std::string> channel(3);
channel.Add("test");

std::optional<std::string> res = channel.Get();
assert(res.value() == "test");
```

With stream operator
```C++
channel << "test";

std::string res;
channel >> res;
```

Golang style channel range iteration.
```C++
LChannel<int> channel;
auto fut = std::async(std::launch::async, [&]{ 
    for (int i = 0; i < 3; ++i) {
        channel << i;
    }
    channel.Close();
});

for (auto const& elem : channel) {
    std::cout << elem << ' ';
}
std::cout << std::endl;
```

## Thread Pool

- ThreadPool<T> : finite task thread pool, if capacity exhausted, block new and wait for existing tasks to be terminated.
- LThreadPool<T> : list based thread pool.

Add new tasks and get return value from future.
```C++
ThreadPool<int> pool;
std::future<int> fut = pool.Add([]{
    int sum = 0;
    for (int i = 0; i < 5; ++i) {
        sum += i;
    }
    return sum;
});

assert(fut.get() == 1 + 2 + 3 + 4);
```

## Wait Group

Wait until all visits are done.

```C++
WaitGroup wg;
wg.Add();

auto fut = std::async(std::launch::async, [&]{
    std::this_thread::sleep_for(2s);
    wg.Done();
});

auto start = std::chrono::steady_clock::now();
wg.Wait();
auto end = std::chrono::steady_clock::now();

std::cout << std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
```

## Select

Channel operation multiplexer, samples from [tick.cpp](./sample/tick.cpp)
```C++
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
```
