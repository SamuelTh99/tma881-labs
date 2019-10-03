# tma881-labs

<!-- For highest grade: After reading the report thoroughly twice, the implementation ideas become clear. -->

## Relevant concepts

<!-- For highest grade:
    - Lists at least four additional concepts from the lecture that hypothetically impact the implementation.
    - Revisits for each concept how it can be accessed in C–code and how it manifests itself in hardware.
    - Provides a clear cut hypothesis for how the concept could impact the implementation. -->

- **Stack allocation**.  We have learned in Assignment 0 that there are two
  types of allocated memory: Stack and heap allocated memory. One acquires
  stack allocated memory by using array declarations inside of functions, as
  opposed to global declarations of arrays.

  Memory allocated on the stack is limited in size, but tends to be faster.
  Moreover, stack allocated memory is thread local and therefore provides an
  opportunity to untangle the mutual impact of parallel threads on one another.
  Consequentially, it is an important consideration to employ stack allocated
  memory in the innermost iteration steps, i.e., the Newton iteration for an
  individual point on the complex plane.

  We plan to test this concept by comparing runtimes of variants of our program
  using stack and heap allocated memory in the innermost iteration steps.

- **Cache locality, memory fragmentation**. The CPU has different memory entities: registers, cache (L1, L2, ...), (main) memory (RAM) and disk; of these, registers is fastest, then cache, then memory and then disk. The CPUs registers are so small that they only hold data for a few clock cykles while the CPU does calculations on them. Thus the CPU constatnly swaps data with its different memory entities. However disk is multiple multiples times slower than memory, which in turn is multiple multiples times slower than cache. Thus it would be a catastrophy if the program has to swap in data from disk, and a smaller catastrophy if it has to swap in data from memory.

When data is swaped in to cache from memory (and to memory from disk) it does so in lines, i.e. it assumes the principle of spatial locality; that if one pice of data is need, adjecent data is probably also needed. The CPU also asumes the principle of temporal locality; that if on pice of data is used once it will probably be used again, however temporal locality will probably not be really relavant here. 
    
Thus the CPU swaps in the requested data together with some fixed size of adjecent data. This makes it possible to avoid much data swaping with memory by programming in a way that makes sure to allocate data in a adjecent maner and to do calculations on all adjecent data that the memory swaped in before doing another swap or moving on.

This will be conciderd and utilized in the program when allocating arrays and when fine-tuning how many calculations each loop and thread will perform before moving on. More on this in the "Thread communication"- and "Static vs dynamic load balancing" section.

- **Inlining**. Fishur

- **Thread communication (mutexes etc.)**. As we learned in the lecture on parallel computation, the runtime of parallel programs is heavily affected by communication between threads. When using shared memory, mutexes need to be used to coordinate memory access in order to make sure that concurrent reads and writes don't corrupt the data. Since a mutex can only be held by a single thread at any time, acquiring a mutex can cause all other threads to be locked. This means that unnecessary usage of mutexes by a single thread can cause an entire program to slow down, which is why great care needs to be had when working with mutexes.

In C, mutexes are used in the following way:

```
pthread_mutex_tsome_mutex some_mutex;
void some_function() {
  pthread_mutex_lock(&some_mutex);
  // Access shared variable.
  pthread_mutex_unlock(&some_mutex);
}
```

In the program that we're writing for this assignment, we will need to use a matrix stored in shared memory to communicate results between the worker threads and the write thread. While the program can be written so that all worker threads write to disjunct sets of indices, the write thread needs to read from all indices, and since reading and writing the same index concurrently is undefined behaviour, we will need to use a mutex to coordinate these accesses.

There are a lot of things to think about when designing a program that uses mutexes. We need to make sure that the program doesn't end up in a state of deadlock, where no thread is able to continue with its execution, or that a single thread starves other threads by rapidly re-aquiring a mutex. We also want to minimize the amount of time where the mutexes are locked, which can be achieved by for example copying the result of the shared variable to a local variable, and then doing computations using the local variable instead of the globally shared one.

- **Static vs dynamic load balancing**. In the lecture on parallel computation, we also learned about static and dynamic load balancing, which are two different ways of balancing work between threads.

Say that we have `r` worker threads. In static load balancing, the input is split in `r` equally sized parts, and each part is given to a thread. When a thread is done with its computation, it will simply exit. This works great if the work load for each part is roughly the same, since all the threads will be done with their computations at around the same time. However, if this is not the case, static load balancing can lead to suboptimal CPU utilization.

In dynamic load balancing, the work is handed out gradually to the worker threads. In the extreme case, we can consider the computation of Newton's method for a single point as a single work load. The main thread would then start `r` threads and give each thread a point. When a thread is done with its computations it will signal this to the main thread, and the main thread will give the thread a new point to work on. This will continue until the roots for all points have been calculated. This method of balancing load will result in better CPU utilization for uneven workloads, but it also incurs additional communication costs. The communication costs can be alleviated by splitting up the work into larger chunks, for example entire rows instead of individual points.

<!-- TODO: Finish this segment. -->

Dynamic load balancing is also necessary when doing computations in a large cluster, since node failures are bound to happen eventually. If static load balancing is used in such an environment, parts of the result will be lost.

- **HDD vs SSD for writing files**. Fishur

## Intended program layout

<!-- For highest grade:
    - After reading the program layout thoroughly twice, the program’s envisioned structure becomes clear.
    - The discussed subtask can probably be implemented in few tens of lines each. -->

Per instructions the program naturally splits into two subtasks: The
computation of the Newton iteration and the writing of results to the two
output files. Each of them will be implemented in a separate function, that are
intended to be run as the main function of corresponding POSIX threads.

The computation of the Newton iteration can be further split up into
***INSERT***

As for the writing to file, we have identified ***INSERT*** independent
subtasks. ***INSERT***

***CONTINUE BY FURTHER SPLITTING UP THE TWO TASKS***
