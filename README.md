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
