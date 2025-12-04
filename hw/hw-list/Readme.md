# HW 1: Lists
## Observing a Multi-Threaded Program
1. Is the program’s output the same each time it is run? Why or why not?

ANS: No, the program's output is not guaranteed to be the same each time it runs. The main reasons are:
* **Thread scheduling non-determinism**: The operating system scheduler determines the order in which threads execute, and this can vary between runs

* **Race condition on common**: Multiple threads increment the shared common variable without synchronization, leading to unpredictable values being printed

* **Concurrent memory access**: Threads access the shared somethingshared string concurrently

2. Based on the program’s output, do multiple threads share the same stack?

ANS: No, **each thread has its own stack**. The output shows different stack addresses for each thread's local variable tid.

3. Based on the program’s output, do multiple threads have separate copies of global variables?

ANS: No, **all threads share the same global variables**. The output shows the same address for common across all threads.

4. Based on the program’s output, what is the value of void *threadid? How does this relate to
the variable’s type (void *)?

ANS: The value of void `*threadid` is the thread ID passed from main() (0, 1, 2, 3, etc.) cast to a `void*` pointer. 

This relates to the type because:

* `pthread_create()` expects a `void*` argument for passing data to threads

* The program casts the integer thread ID to `void*` to pass it

* Inside `threadfun()`, it's cast back to long for use


5. Using the first command line argument, create a large number of threads in pthread. Do all
threads run before the program exits? Why or why not?

ANS: When creating a large number of threads (e.g., `./program 1000000`):

No, **not all threads may run before the program exits**, because:

* **Resource limits**: The system may have limits on the maximum number of threads per process

* **Memory exhaustion**: Each thread requires stack space (typically 2-8MB by default)

* **Scheduling delays**: With many threads, some may not get scheduled before the main thread's pthread_exit() allows the process to exit

* **Creation failures**: pthread_create() may fail for later threads due to system resource constraints


## Additional Questions
I write a script `test.sh` to compare the `./pwords` and `./lwords` speed and output content. To use it, just run `./test.sh`.   

1. Briefly compare the performance of lwords and pwords when run on the Gutenberg dataset. How might you explain the performance differences?

ANS: pwords (parallel version) is no betten than lwords (sequential version) on the Gutenberg dataset (the sequential version completes the work ~1.06x faster than parallel version), with the possible reason given follows:

* Bottleneck Analysis:
    * In the implementation of pwords (`word_count_p.c`), a global mutex lock (`wclist->lock`) is used to protect the entire linked list.
    * Whenever any thread wants to add or look up a word, it must acquire this lock.
* Serialization of Execution:
    * The lookup operation in the linked list is $O(N)$ (linear time). As the number of words increases, traversing the list becomes increasingly slow. Since this part of the code is protected by a lock (critical section), only one thread can execute add_word at any given time. This means that even though multiple threads are created, they are actually waiting in line to enter the critical section. As a result, the program essentially degenerates into serial execution.
* Additional Overhead:
    * Not only does pwords fail to take advantage of parallelism, it also incurs extra overhead:
        * Thread creation and switching overhead: The operating system needs resources to manage threads.
        * Lock overhead: Acquiring and releasing locks, as well as threads being blocked and woken up for the lock, all take time.

2. Under what circumstances would pwords perform better than lwords? Under what circumstances would lwords perform better than pwords? Is it possible to use multiple threads in a way that
always performs better than lwords?

ANS: 
**pwords performs better than lwords when**:

* Processing many files (good workload distribution)

* Running on multi-core/multi-CPU systems

* Files are large enough to amortize thread creation overhead

* I/O subsystem can handle parallel read operations efficiently

* CPU-bound processing (hashing, counting) dominates over I/O

**lwords performs better than pwords when**:

* Processing very few files (especially just one large file)

* Running on single-core systems (no benefit from parallelism)

* Files are very small (thread overhead dominates)

* System has heavy load already (threads compete for resources)

* I/O bottlenecks prevent parallel I/O benefits

* Memory constraints make parallel processing inefficient

**No, it's not possible for multiple threads to always perform better than the sequential version**. The reasons are:

* Amdahl's Law: The maximum speedup is limited by the sequential portion of the program

* Overhead costs: Thread creation, synchronization, and communication have inherent costs

* Resource contention: Multiple threads competing for shared resources (I/O, memory bandwidth)

* Diminishing returns: Beyond optimal thread count, additional threads increase overhead without benefits

* Worst-case scenarios: Some workloads are inherently sequential or have dependencies that prevent parallelization

**However, with proper implementation, multi-threading can provide better performance in most practical scenarios** by:

* Using dynamic workload balancing

* Optimizing thread pool sizes based on system capabilities

* Implementing efficient synchronization mechanisms

* Considering both file count and size when parallelizing

The key is **adaptive parallelism** - using sequential processing when it's more efficient and parallel processing when the workload and system can benefit from it.


