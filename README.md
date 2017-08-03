# MemoryLimits
Test memory limits, e.g., the effect of ulimit

## What is it?
Two applications are provided, `alloc` and `mem_limit`.  Both serve a
similar purpose, i.e., testing memory limits set by `ulimit` or `cgroups`.
`alloc` is a simple serial application, while `mem_limit` is hybrid
MPI/OpenMP.

## How to build?
The provided make file in each of the directories `alloc` and `mem_limit`
should build the application, i.e.,
```bash
$ make
```

## How to use?
### `alloc`
The `alloc` command takes a few options:
* `-maxMem <bytes>`: maximum number of bytes to be allocated and written
    to memory.
* `-incr <bytes>`: number of bytes to allocate and write in each step,
    steps are separated by sleep.
* `-sleep <seconds>`: seconds to sleep between increments and after final
    step.

### `mem_limit`
This application can run with a single or multiple processes, each process
can have one or more threads. Command line options allow for the
configuaration of a homogeneous run, i.e., all processes and threads will
have the same memory usage pattern.
```bash
$ mpirun -np 3 ./mem_limit -t 2 -m 4gb -i 1gb -s 100ms -l 1s
```
This will start the application with 3 processes, 2 threads each.  Each
thread will allocate up to 4 GB of RAM, in incremental steps of 1 GB.
After each allocation step, there is a 100 ms pause, and when all memory
is allocated, the application pauses for a second.

It is also possible to configure the pattern for each process and even
threads individually using a configuration file.
```bash
$ mpirun -np 3 ./mem_limit -f conf/inhomogenious_mpi.conf -l 5s
```
This configuration specifies that the first process should use more memory
than the others.
```
# process 0 consumes more memory than the others
1;100mb+50mb+1s
1;50mb+25mb+1s
```
Each process will have a single thread, the first process will use a
maximum of 100 MB RAM, in incremental steps of 50 MB, all other
processes will use only a maximum of 50 MB RAM in 25 MB increments.

Note that the number of threads can be specified on a per-process basis,
and that each thread can have its own memory allocation pattern.
```
2;100mb+50mb+100ms:50mb+10mb+50ms
3;25mb+25mb+200ms
```
The first process will have two threads, the first using 100 MB of RAM,
the second 50 MB.  All other processes will have three threads, each using
25 MB of RAM.

It is not required to specify the information for all processes and/or
threads explicitely.
