# C++: templates

## Templates – core language feature

Motivation: overloads like the pair of functions below.

```c++
int max(int a, int b) {
    return a > b ? a : b;
}

double max(double a, double b) {
    return a > b ? a : b;
}
```

The code is the same, only the type changes. This can be generated automatically,
and will work for anything that has a "greater than" operator:

```c++
template <typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

int main() {
    // explicitly specifying the type at the call site
    std::cout << max<int>(5, 8) << std::endl;

    // but this can be omitted, the compiler will deduce:
    std::cout << max(5, 8) << std::endl;        // T = int
    std::cout << max(3.14, 8.5) << std::endl;   // T = double

    // compiler error: cannot deduce template argument
    // std::cout << max(6, 6.7) << std::endl;   // int? double?

    // it compiles when the type is explicity specified
    std::cout << max<double>(6, 6.7) << std::endl;

    std::string a = "apple", b = "banana";
    std::cout << max(a, b) << std::endl;        // T = std::string
}
```

Templates:

    - Code is generated at compile time, therefore you must know the template argument at compile time.
    - Zero runtime cost!
    - However it may lead to code bloat (lot of different versions are generated). This may be important for embedded systems.

Class templates also exist. A dynamic array like `std::vector` is:

```c++
template <typename T>
class MyVector {
  private:
    size_t size;
    T *data;
  public:
    explicit MyVector(size_t s);
    ~MyVector() {
        delete[] data;
    }
    /* ... */
};

// Non inline function: note that the method of a class template is
// a template itself
template <typename T>
MyVector<T>::MyVector(size_t s) {
    size = s;
    data = new T[size];
}
```

When instantiating class templates, the template argument must be explicitly
specified. Otherwise the compiler cannot figure it out. See the example
below: what would `std::vector v(100)` mean? A vector of 100 elements, but
what is the elements' type?

```c++
std::vector<int> v(100);
v[12] = 37;
v.resize(200);
```

Since C++17, the type can be figured out in some cases:

```c++
std::vector v = { 1.2, 3.4, 5.6 };      // obviously vector of double
```

But you have to provide the compiler with a deduction guide
(CTAD = class template argument deduction guide):

```c++
#include <iostream>

template <typename T>
class MyVector {
  private:
    // ...
  public:
    MyVector(std::initializer_list<T> elements) {
        size = elements.size();
        data = new T[size];
        // ...
    }
} 

// Class template argument deduction guide:
// a MyVector initialized with std::initializer_list<Something> elements
// should have the template argument Something.
// Eg. { 1.2, 3.4, 5.6 } is an initializer list of doubles,
// therefore the vector will be MyVector<double>.
template <typename S>
MyVector(std::initializer_list<S> elements) -> MyVector<S>;
```


## Templates – specialization

Sometimes we want to change the behavior of the function for some
template arguments. Consider the `equals()` function template below:

```c++
template <typename T>
bool equals(T const & a, T const & b) {
    return a == b;
}
```

For real numbers we prefer doing equality checking using some
small epsilon value. This is a special case, where the comparison
will not work with the usual == operator. Therefore we specialize
the function:

```c++
template <>
bool equals<double>(double a, double b) {
    return std::abs(a-b) < 1e-6;
}
```

The syntax is: a) `template <>` as the first line, which tells the
compiler that the function definition will be a specialization, b)
`equals<double>` as the function name, which states the concretized
template arguments for the `equals` function template.

The compiler will automatically select the base template or the
specialization, based on the type:

```c++
std::cout << equals(5, 6);
std::cout << equals(5.0, 5.0000001);
```

Remark 1: in the code above `std::abs` is used deliberately.
A simple `abs` will not work, as it is the `int abs(int)` function
inherited from C. The C++ versions reside in the `std` namespace,
and `std::abs` is an overload set for different types.

Remark 2: argument passing semantics have changed for this specialization.
Here we have `double`, pass by value, instead of `T const &`, pass by reference.
This is ok, a specialization is allowed to change the semantic, and
passing by value is preferred for built-in types (very small objects).
But be sure not to surprise the callee!




## STL algorithms

STL = Standard Template Library, it has a lot of containers and basic algorithms.

All algorithms work with "pointers". For example:

```c++
int array[10] = { 8, 5, 7, 9, 3, 5, 7, 1, 3, 2 };

// Sorts the array. We specify a half open range:
// - array = &array[0] -> first element (closed on the left hand side)
// - array+10 -> element after last one (open on the right hand side)
std::sort(array, array+10);
```

Also:

```c++
bool is_even(int i) {
    return i%2 == 0;
}

int array[5] = { 8, 9, 6, 4, 5 };
std::cout << std::count_if(array, array+5, is_even);     // Will print 3, as {8, 6, 4} are even numbers
```

The function template `std::count_if` looks like this:

```c++
template <typename ITER, typename PRED>
unsigned count_if(ITER begin, ITER end, PRED p) {
    unsigned cnt = 0;
    for (ITER it = begin; it != end; ++it)
        if (p(*it))
            cnt += 1;
    return cnt;
}
```

In the example above, the `count_if` template used with our `array`
of integers and `is_even` function is specialized automatically like this:

```c++
template <>
unsigned count_if<int*, bool(*)(int)>
(
    int *begin,
    int *end,
    bool (*p)(int)
)
{
    unsigned cnt = 0;
    for (ITER it = begin; it != end; ++it)
        if (p(*it))
            cnt += 1;
    return cnt;
}
```

That is:

- `begin` and `end` are `int*` pointers,
- predicate `p` has type `bool (*)(int)`, a pointer to the function.

Important:

- The array is traversed with pointer arithmetics, `++it` is the next integer in memory.
- `it = begin` and `it != end` control the loop, that is why we have closed interval
    on the left (begin) and open interval on the right (end). `begin` points to the
    first element to process, `end` points one past the end of the container.
    This is how we can have empty containers, `begin == end`.
- `p` pointer points to `is_even`, but as this fact is known during compile time,
    the body of `is_event` will be inlined/optimized to the call site. This code is FAST,
    the abstraction is optimized away by the compiler.




## Duck typing

Consider how we use the predicate in `count_if`:

```c++
if (p(*it))
    cnt += 1;
```

In the `p(*it)` expression `p` could be _anything_, not just a pointer to a function.
Anything that can be used as a function, and returns a boolean. Maybe a function
object:

```c++
class GreaterThan {
  private:
    int limit;
  public:
    GreaterThan(int limit) {
        this->limit = limit;
    }
    bool operator() (int i) const {
        return i > limit;
    }
};

int array[5] = { 8, 9, 6, 4, 5 };
std::cout << std::count_if(array, array+5, GreaterThan(6));   // prints 2, as {8, 9}
```

How it works:

- Template arguments deduced are: `int*` and `GreaterThan`.
- A temporal object `GreaterThan(6)` is created, this wellbe argument `p` of `count_if`.
- Inside function, `p(*it)` is interpreted as `p.operator()(*it)`, because
    the operator syntax is resolved to a method invocation on the objects.

This could also be a lambda expression (ad-hoc function, we'll discuss these later):

```c++
int array[5] = { 8, 9, 6, 4, 5 };
std::cout << std::count_if(array, array+5, [] (int i) { return i <= 5; });
```

The `GreaterThan` class could be a template itself, because we have other numeric
or otherwise comparable types. The only thing important is that the type has
a greater than `>` operator. The `GreaterThan<int>(6)` expression has two different
kind of parenthesis, `<int>` instantiates a class using the class template
(creates the specialization where `T = int`), and then `(6)` calls the constructor:

```c++
template <typename T>
class GreaterThan {
  private:
    T limit;
  public:
    GreaterThan(T const & limit) {
        this->limit = limit;
    }
    bool operator() (T const & i) const {
        return i > limit;
    }
};

int array[5] = { 8, 9, 6, 4, 5 };
std::cout << std::count_if(array, array+5, GreaterThan<int>(6));   // prints 2

double array2[5] = { 8.1, 9.2, 6.5, 4.3, 5.1 };
std::cout << std::count_if(array2, array2+5, GreaterThan<double>(6.02));   // prints 3
```

C++17 CTAD can be used here with the proper deduction guide:

```c++
template <typename T>
GreaterThan(T limit) -> GreaterThan<T>;


int main() {
    int array[5] = { 8, 9, 6, 4, 5 };
    std::cout << std::count_if(array, array+5, GreaterThan(6));   // note no <int>

    double array2[5] = { 8.1, 9.2, 6.5, 4.3, 5.1 };
    std::cout << std::count_if(array2, array2+5, GreaterThan(6.02));   // note <double> not required
}
```

This is only for convenience, the meaning and the compiled code is the same.



## STL containers

Just a short list:

- `std::vector<T>`: dynamically allocated, resizable array
- `std::array<T, size>`: array with static (compile-time known) size. Better than `T array[size]`, as it can be passed by value.
- `std::list<T>`: doubly linked list
- `std::set<T>`
- `std::map<KEY, VAL>`: key-value map
- `std::stack<T>`
- `std::queue`, `std::deque`, `std::priority_queue`, `std::multimap`, `std::multiset`, `std::hash` function, `std::unordered_map`, `std::unordered_set`...




## Iterators

Function body of `count_if` once again:

```c++
for (ITER it = begin; it != end; ++it)
    if (p(*it))
        cnt += 1;
```

Think about duck typing again. Here `ITER` is not necessarily a pointer, it can be an
object with properly overloaded operators. Has to be copiable (`ITER it = begin`),
comparable (`it != end`), incrementable (`++it`) and dereferable (`*it`).

This is how we make iterators in C++. The iterator knows the internal structure of the container,
and provides a pointer-like interface to access the elements. For a linked list, the code is
the same:

```c++
std::list<double> mylist = { 1.1, 2.3, 4.5 };
std::list<double>::iterator it;
for (it = mylist.begin(); it != mylist.end(); ++it)
    std::cout << *it << std::endl;
```

Conventions for iterators and containers:

- Iterator type is always `container_type::iterator` and `container_type::const_iterator`
- Iterators have the same interface as pointers.
- The container has methods named `.begin()` and `.end()`, which specify the half open range `[begin; end)`.
- Usually there are `.cbegin()` with `.cend()` (for const access), and `.rbegin()` with `.rend()` (reverse iterator) as well.

Iterators are usually created in the for loop header, as an initialized objects.
This way `auto` can be used, as the compiler can guess the type of the iterator,
which is obviously the same as that one of the `begin()` method.

```c++
std::list<double> mylist = { 1.1, 2.3, 4.5 };
for (auto it = mylist.begin(); it != mylist.end(); ++it)
    std::cout << *it << std::endl;
```

Traversing a container is a frequently used operation, and there is a shorthand for it called the range-based for loop:

```c++
std::list<double> mylist = { 1.1, 2.3, 4.5 };
for (double d : mylist)
    std::cout << d << std::endl;
```

The compiler will generate the same code. We usually write this using `auto`:

```c++
std::list<double> mylist = { 1.1, 2.3, 4.5 };

// note that d is a reference, so that we can modify the element in the container
for (auto & d : mylist)
    d += 10;

// we use const &, if we neither want to change, nor copy the element:
for (auto const & d : mylist)
    std::cout << d << std::endl;
```

All STL algorithms use iterators:

```c++
std::list<double> mylist = { 1.1, -2.3, 4.5, 3.7 };

// find the greatest number, return an iterator
auto max_it = std::max_element(mylist.begin(), mylist.end());
std::cout << "Maximum: " << *max_it << std::endl;

// find the first negative element, return an iterator.
// if none found, return end iterator
auto first_neg = std::find_if(mylist.begin(), mylist.end(),
                              [] (auto d) { return d < 0; });
if (first_neg == mylist.end())
    std::cout << "No negative number in container" << std::endl;
else
    std::cout << "The first negative number is: " << *first_neg << std::endl;
```

This is the power of STL: you can use any container with any algorithm,
even your own containers and own iterators.
