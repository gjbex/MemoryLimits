# MemoryLimits
Test memory limits, e.g., the effect of ulimit

## How to build?
The provided make file should build the application, i.e.,
```bash
$ make
```

## How to use?
The `alloc` command takes a few options:
* `-maxMem <bytes>`: maximum number of bytes to be allocated and written
    to memory.
* `-incr <bytes>`: number of bytes to allocate and write in each step,
    steps are separated by sleep.
* `-sleep <seconds>`: seconds to sleep between increments and after final
    step.
