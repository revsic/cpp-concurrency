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

- Channel<T> : finite capacity channel, if capacity exhausted, block channel and wait for free space.
- UChannel<T> : infinite capacity channel, implemented by std::list.

Add and get from channel.
```C++
Channel<std::string> channel(3);
channel.Add("test");

std::optional<std::string> res = channel.Get();
assert(res.value() == "test");
```

Golang like range iteration.
```C++
UChannel<int> channel;
auto fut = std::async(std::launch::async, [&]{ 
    for (int i = 0; i < 3; ++i) {
        channel.Add(i);
    }
});

for (auto const& elem : channel) {
    std::cout << elem << ' ';
}
```
