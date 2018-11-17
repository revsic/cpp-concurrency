# cpp-concurrency

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
Channel<int> channel;
WaitGroup wg;
```

## Sample

Calculate directory size concurrently.
```
cd sample && mkdir build;
pushd build;
cmake .. && make;
./concurrency_example $YOUR_DIRECTORY
popd
```

## Channel

- Channel<T> : finite capacity channel, if capacity exhausted, block channel and wait for space.
- UChannel<T> : list like channel.

Add and get from channel.
```C++
Channel<std::string> channel(3);
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
UChannel<int> uchannel;
auto fut = std::async(std::launch::async, [&]{ 
    for (int i = 0; i < 3; ++i) {
        uchannel << i;
    }
    uchannel.Close();
});

for (auto const& elem : uchannel) {
    std::cout << elem << ' ';
}
std::cout << std::endl;
```

## Thread Pool

- ThreadPool<T> : finite task thread pool, if capacity exhausted, block new and wait for existing tasks to be terminated.
- UThreadPool<T> : thread pool implemented by task lists.

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
std::cout << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;
```
