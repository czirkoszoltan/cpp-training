# Threads

## Threads and mutexes

C++ thread management is very similar to other languages. You see the same basic concepts and helper classes.
These only abstract the underlying operating systems' services, and provide a C++ interface.
So you have `thread`, `detach`, `join`, `mutex`, `condition_variable` etc.

You can start a new thread with the ctor of `std::thread`. Arguments are the function to be called and its
arguments. You have to store the thread object; by the time the dtor runs, the thread must be `join()`-ed or
`detach()`-ed. The code below starts 20 independent threads, the handles of which are stored in a vector

```c++
#include <iostream>
#include <thread>
#include <vector>
 
void mythread(int i) {
    std::cout << "Hello from the thread: " << i << std::endl;
}
 
int main() {
    std::vector<std::thread> threads;
    
    for (int i = 1; i <= 20; ++i)
        threads.push_back(std::thread(mythread, i)); // mythread(i)
    
    for (auto & t : threads)
        t.join();
}
```

The above code can generate garbage on the output, because the printing operations all access the same
output stream. You can use `std::mutex` to synchronize; `m.lock()` and `m.unlock()`. Of course the `mutex`
object must be common, for example as a global variable:

```c++
std::mutex m;
 
void mythread(int i) {
    m.lock();
    std::cout << "Hello from the thread: " << i << std::endl;
    m.unlock();
}
```

Or you can pass it by reference. Calling `std::ref` is important here! Anyway without it
the code won't compile, as `std::mutex` has no copy ctor.

```c++
void mythread(int i, std::mutex & m) {
    m.lock();
    std::cout << "Hello from the thread: " << i << std::endl;
    m.unlock();
}

/* ... */
 
std::mutex m;
for (int i = 1; i <= 20; ++i)
    threads.push_back(std::thread(mythread, i, std::ref(m)));
```

The thread can be a lambda function, which can capture the mutex by reference:

```c++
std::mutex m;
for (int i = 1; i <= 20; ++i)
    threads.push_back(std::thread([i, &m] () {
        m.lock();
        std::cout << "Hello from the thread: " << i << std::endl;
        m.unlock();
    }));
```

Generally we do not `lock()` and `unlock()` the thread manually. It's better to use
a `std::lock_guard`. Its ctor locks, and dtor unlocks a mutex. This way we cannot
forget to do so, and also we can throw exceptions freely – this is RAII (resource acquisition
is initialization) for the mutexes.

The function can be implemented like this

```c++
std::mutex m;
for (int i = 1; i <= 20; ++i)
    threads.push_back(std::thread([i, &m] () {
        std::lock_guard<std::mutex> lg(m);
        std::cout << "Hello from the thread: " << i << std::endl;
    }));
```

A well-known example: bank account with multiple users.

```c++
class BankAccount {
  private:
    std::mutex m;
    int balance = 0;
  public:
    void deposit(int amount) {
        std::lock_guard<std::mutex> lg(m);  /* RAII lock guard */
        balance = balance + amount;
    }
    /* withdraw money, if sufficient funds.
     * otherwise return 0. */
    int withdraw(int amount) {
        std::lock_guard<std::mutex> lg(m);  /* RAII lock guard */
        if (amount <= balance) {
            balance = balance - amount;
            return amount;                  // unlock
        }
        else {
            return 0;                       // unlock
        }
    }
};
```

There is no synchronize annotation in C++. However you can create it with a very
simple function:

```c++
template <typename MUTEX, typename TEENDO>
inline void synchronize(MUTEX & m, TEENDO && t) {
    std::lock_guard<MUTEX> lg{m};
    t();
}

synchronize(m, [&] {
    /* critical section here */
});
```

Further reading:

- `std::timed_mutex`.
- `std::recursive_mutex`.
- `std::timed_recursive_mutex`.



## Atomic variables

For built-in types, there are atomic classes. These can be used if the operations are very simple (increment, decrement), because
they are faster. A simple counter to know how many persons are in a shop:

```c++
#include <atomic>
 
class ThreadSafeCounter {
  private:
    std::atomic<int> counter = 0;
  public:
    void increment() { counter++; }
    void decrement() { counter--; }
    int get() const { return counter.load(); }
};
```

Beware! The operations are only atomic if you use one operator. In the following code, there are two operations (first: read from i, second: write to i), so the operation will not be atomic.

```c++
void bogus_function(std::atomic<int> & i) {
    i = i*2 + 1;
}
```

The above is essentially this:

```c++
void hibas(std::atomic<int> & i) {
    i.store(i.load()*2 + 1);
}
```

This has the same bug:

```c++
void bogus_function(std::atomic<int> & i) {
    i *= 2;
    i += 1;
}
```

Because a thread switching can take place between the two code lines. If the operations are complex, use a mutex instead.



## std::future

Sometimes we want to start a thread, so it runs on another CPU core (for speed). The thread will return a value later. At other times
we have a task to fulfil, but later only - we have a function and its arguments to run, and we put these in a todo list to run
later, or omit altogether.

We call the delayed execution of a function call a future, or a promise, or sometimes a delayed function call. In C++, we can
do such call with `std::async()`, which returns a handle of the type `std::future<T>` (where T is the return type of the function).
To get the return type, we simply use the `get()` method: if the call is delayed, it is run, or if it is run a different thread,
then the thread is joined. With `async` and `future` you can avoid building your own infrastructure for synchronization.

The first argument of `std::async` is the launch policy. If it is `std::launch::async`, then a new thread will be started. If it is
`std::launch::deferred`, the same thread is used, however the function is only executed at `.get()`. If both are specified, or
the argument is omitted, implementation-defined behavior will occur.

```c++
#include <iostream>
#include <algorithm>
#include <numeric>
#include <thread>
#include <future>
 
int parallel_sum(int *begin, int *end) {
    int len = end-begin;
    if (len < 1000)
        return std::accumulate(begin, end, 0);
    
    int *mid = begin + len/2;
 
    auto handle = std::async(std::launch::async, parallel_sum, begin, mid);
    int second_part = parallel_sum(mid, end);
    int first_part = handle.get();
 
    return first_part + second_part;
}
 
int main() {
    int arr[10000];
    std::generate(std::begin(arr), std::end(arr), [] { return rand()%100; });
    std::cout << parallel_sum(std::begin(arr), std::end(arr));
}
```

Beware: the last three lines of the function cannot be simplified to `return handle.get() + parallel_sum(mid, end)`! The evaluation
order in this case would be undefined, and if the compiler decided to call `handle.get()` first, you lose parallel execution.
The above code is correct, because sequence points created with code lines will force the `parallel_sum(mid, end)` recursive call
to be called before invoking `get()`.
