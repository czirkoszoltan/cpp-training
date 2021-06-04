# Template metaprogramming

## Policies and strategies

Consider the following example.

```c++
int main() {
    MyVector<int> v;
    vv.resize(10);
    v[5] = 123;
    v[13] = 95;
}
```

The vector has a size of 10, it can be indexed from zero to nine (inclusive).
Therefore the `v[5]` member access is fine, however `v[13]` is out of bounds.
The question is: how can the vector handle this situation?

- It can ignore the indexing error; it is up to the caller to use proper indices.
- It can throw an exception.
- It can automatically resize itself.
- It can change the index to wrap around.
  

What if – as an author of the vector class – we wanted to support all these
strategies, maybe more? The behavior of the vector is only different in
`operator[]`, otherwise all other parts of the code (memory management etc.)
are identical. We should be able to make the vector configurable without
copy-pasting the code.

```c++
template <typename T>
class MyVector {
    private:
        std::vector<T> data;            // identical
    public:
        void resize(size_t s) {         // identical
            data.resize(s);
        }
        size_t size() const {           // identical
            return data.size();
        }
        T & operator[] (size_t idx) {   // make this configurable
            return data[idx];
        }
};
```

The solution is to create a new class which will be responsible for
checking the index, and use that class as an argument to configure the
vector. Like this:

```c++
class VectorIgnoreBounds {
    public:
        template <typename V>
        static void check_index(V & vec, size_t & idx) {
            // do nothing
        }
};

class VectorThrowException {
    public:
        template <typename V>
        static void check_index(V & vec, size_t & idx) {
            if (idx >= vec.size())
                throw std::out_of_range("index out of bounds");
        }
};
```

Before the actual indexing, the vector will call the `check_index` function:

```c++
template <typename T, typename IDXSTRATEGY>
class MyVector {
    public:
        /* ... */
        
        T & operator[] (size_t idx) {
            IDXSTRATEGY::check_index(*this, idx);
            return data[idx];
        }
};
```

Note that the indexing strategy become a template argument of the vector.
So now it can be used like this:

```c++
int main() {
    MyVector<int, VectorThrowException> v;
    v.resize(10);
    v[5] = 123;
    v[13] = 95;
}
```

What happens at the instantiation and the `operator[]` function call?

- The vector is instantiated with the `VectorThrowException` template argument.
- When `operator[]` is called, `IDXSTRATEGY::check_index` will be called as well.
- In this case, it is equivalent to `VectorThrowException::check_index`.
- This function will check the index, taking the size of the vector into consideration.
    (That's why it got the vector itself as an argument.)
- If the index is out of bounds, an exception is thrown - and it is not caught in `operator[]`.

What happens if the vector is instantiated with `VectorIgnoreBounds`?
`VectorIgnoreBounds::check_index` is also called, however it is an empty function,
so the index will not be checked.

The other two strategies would be:

```c++
class VectorWrapAroundIndexing {
    public:
        template <typename V>
        static void check_index(V & vec, size_t & idx) {
            idx %= vec.size();
        }
};

class VectorResizeOnIndex {
    public:
        template <typename V>
        static void check_index(V & vec, size_t & idx) {
            if (idx >= vec.size())
                vec.resize(idx + 1);
        }
};
```

These explain why it was beneficial to pass the vector and the index by reference,
although it is not necessary in the other strategy classes (because of duck typing).

This whole technique is essentially an implementation of the OOP strategies
design pattern, but in compile time. Note that we use templates, and that means that
the compiler sees *all function bodies*, therefore it is able to optimize away
the function call mechanism – in the latter example, the empty call is optimized away
altogether. This technique has *zero runtime cost*.

A little inconvenience here is that the vector has two template arguments now,
the element type and the index strategy. However, a default argument solves that problem:

```c++
template <typename T, typename IDXSTRATEGY = VectorIgnoreBounds>
class MyVector {
    /* ... */
};

int main() {
    MyVector<int> v;        // IDXSTRATEGY will default to VectorIgnoreBounds
}
```


## Variadic template functions

Maybe the most important modern C++ (post-C++11) language feature regarding templates
is variadic template functions and classes.

Consider a logging function, something like `log(message, other, arguments, to, print...)`.
This will be modeled by the `print()` function below:

```c++
int main() {
    print("apple", 1, 3.4);
}
```

This function can have any number of arguments of any type: strings, numbers etc. anything
that can be converted to a string and written to the log file. In C++98, we had to use
overloads to solve the problem:

```c++
template <typename T1>
void print(T1 arg1);

template <typename T1, typename T2>
void print(T1 arg1, T2 arg2);

/* ... */
```

Auto-generated source code was very common. Since C++11, we can used variadic template
functions:

```c++
template <typename ... TYPES>
void print(TYPES ... args) {
    /* implemented later */
}
```

Some remarks on the syntax. The `...` notation for the template argument means that there
can be more types, from zero to infinity. So the function can be instantiated with
`print<int>` or maybe with `print<std::string, double>` or even `print<>`.
The function arguments work the same way: `TYPES ... args` means that there can be many
arguments of the aforementioned types. As the compiler can deduce the template
arguments from the actual function call, `print("apple", 1, 3.4)` will instantiate
the template as `print<char const *, int, double>` as usual.

### Unpacking in a function call

How do we implement this function? `args` is a tricky thing: the standard calls it
an *argument pack*, and it is not a variable. It represents the arguments of the function,
however once cannot access the arguments one by one, because they have different types.
(Ie. there is no such thing as `args[i]`, like `args[0]`, `args[1]` in a loop, because
each argument has a different type, and C++ is a statically typed language.)

There are two things we can do with an argument pack:

- Get its size (argument count) using `sizeof...(args)`.

- Use it in a context where values can be enumerated with commas between them.

For the latter one, the function call is an example for such a context. However
calling a helper function like `helper(args...)` would not help, as we would have the
same problem in that one. The simplest solution is to divide the problem into two:
handle the first argument separately, then handle the other arguments recursively:

```c++
template <typename HEAD, typename ... TAIL>
void print(HEAD head, TAIL ... tail) {
    std::cout << head;
    print(tail...);
}

int main() {
    print("apple", 1, 3.4);
}
```

In this example, `print<char const *, int, double>` is instantiated, therefore
`HEAD = char const *` and `TAIL = int, double`, so the
function call takes the form:

```c++
template <>
void print<char const *, int, double>(char const * head, /* int, double */ ... tail) {
    std::cout << head;
    print(tail...);
}
```

Now we know the exact type of the first argument, and it is stored in the
ordinary variable named `head`. After printing it, we continue by printing the
other values.

Every recursive function should have a base criterion. The `print()` function above
is buggy, nothing stops the recursive calls. This problem surfaces as a *compile
error*, because after printing the last argument, `print()` would be called with
*no arguments*, and the compiler stops. The particular problem from the viewpoint
of the compiler is that it cannot deduce the type `HEAD` from the call `print()`,
as it should have at least one argument. This problem can be solved using an
overload for `print()`:

```c++
void print() {
}

template <typename HEAD, typename ... TAIL>
void print(HEAD head, TAIL ... tail) {
    std::cout << head;
    print(tail...);
}
```

This way, after printing the last argument, the ordinary function `print()`
will be selected by the overload resolution mechanism for empty `tail...`.

### Unpacking in an array

One can also unpack an argument pack into an array, because that is also a context
in the source code where a comma-separated list of values can go. However,
an array is a container for elements *of the same type*, and an argument pack is
usually no such thing.

Even if the arguments are of different types, sometimes we can convert them to a
common type. For example in the case of printing, we can convert all arguments to
strings, because finally they are all emitted as a stream of characters. Our
`print()` function can be implemented like this:

```c++
template <typename T>
std::string tostring(T const & obj) {
    std::ostringstream os;
    os << obj;
    return os.str();
}

template <typename ... ARGS>
void print(ARGS ... args) {
    std::string strings[] = { tostring(args)... };
    for (auto const & s : strings)
        std::cout << s;
}

int main() {
    print("apple", 1, 3.4);
}
```

Let's explain each part:

- `std::string strings[] = { tostring(args)... };` here are the arguments
converted to strings. An array of strings is created, and it is initialized
with the return values from the `tostring` function. Note that the `...`
argument unpacking operator is not after `args...`, but after the `tostring(args)...`
function call. Therefore there is not one function call like `tostring(arg1, arg2, arg3)`,
but rather the function is called once for each argument, like `tostring(arg1), tostring(arg2)` etc.

- This can also explain the function signature `void print(ARGS const & ... args)`.
This means that all arguments are received as const references, as the `...` is
after the `const &`, so `const &` will be applied to all arguments.

- `std::string tostring(T const & obj)` a simple template function to convert anything
to a string, provided the type has the proper stream inserter `<<` operator. This also
exists as a library function named `std::to_string`.

The helper function could be "hidden" in `print()` using a lambda:

```c++
template <typename ... ARGS>
void print(ARGS const & ... args) {
    auto tostring = [](auto const & obj) {
        std::ostringstream os;
        os << obj;
        return os.str();
    };

    std::string strings[] = { tostring(args)... };
    for (auto const & s : strings)
        std::cout << s;
}
```

However this is not really C++-ish.

The technique outlined here makes it easy to access the arguments multiple times
using simple indexing. Eg. one can implement a `print()` function with named (numbered)
arguments in the format string. The arguments can be reordered or used multiple times:

```c++
template <typename ... ARGS>
void print(char const *fmt, ARGS const & ... args) {
    auto tostring = [](auto const & obj) {
        std::ostringstream os;
        os << obj;
        return os.str();
    };

    std::string strings[] = { tostring(args)... };
    for (size_t i = 0; fmt[i] != '\0'; ++i)
        if (fmt[i] == '%')
            std::cout << strings[fmt[++i] - '1'];
        else
            std::cout << fmt[i];
}

int main() {
    print("%2 is %1 years old.\n", 37, "John");
    print("%1 x %1 = %2.\n", 5, 25);
}
```

### Fold expressions

In this basic example, a C++17 folding expression can be used:

```c++
template <typename ... ARGS>
void print(ARGS const & ... args) {
    (std::cout << ... << args);
}
```

This can be a solution if all the arguments are to be processed with the same operator.
The folding expression `(0 + ... + args)` would add all the arguments, as would
`(args + ...)`, because 0 is the "neutral" element for addition (the zero element for integers).
The parenthesis around the fold expression is mandatory, it is part of the syntax.

### Language hacks

This will also print all the arguments (but certainly would not pass a code review):

```c++
template <typename ... ARGS>
void print(ARGS const & ... args) {
    using swallow = int[];
    swallow{ (void(std::cout << args), 0)..., 0 };
}
```

`swallow` here is a temporary array of integers `int[]`, which is used only to have
a context where argument unpacking can take place. The `swallow` typedef is used
because `int[]{ ... }` would be incorrect syntax.

All the arguments are fed to `std::cout` via the `<<` operator. The first `,0` is
required, because the return values of printing expressions are `std::ostream &`-s,
but the temporary array contains integers. `x, 0` is the comma operator here.
The `void` cast will avoid problems if the expression would return some value
for which the comma operator is overloaded. It creates an empty (void) value,
for which you cannot overload this operator. The second `,0` is there to avoid
a compile error if the argument pack is empty.

## std::make_unique, std::forward

The built-in `std::make_unique` function has two tasks:

- It creates an object of type `T`, using the `new` operator.
- It wraps the raw pointer in an `std::make_unique` smart pointer.

```c++
template <typename T>
std::unique_ptr<T> my_make_unique() {
    return std::unique_ptr<T>(new T());
    //     ^^^ wrapping       ^^^ creating
}
```

In the above implementation, arguments are missing, the function
always calls the default ctor of `T`. The standard implementation allows from
any number of constructor arguments. Those can be implemented by a variadic
template function, however first let us consider the problems of one single
argument.

```c++
template <typename T, typename ARG>
std::unique_ptr<T> my_make_unique(ARG arg) {
    return std::unique_ptr<T>(new T(arg));
}

int main() {
    auto s = my_make_unique<std::string>("hello world");
}
```

So far, so good. In the example, the type of the created object will
be `T = std::string`, as it is explicitly specified with `<std::string>`.
The type of the argument will be `ARG = char const *`, it is deduced by
the compiler from the call.

However, imagine that we are passing a string object as an argument.
In that case, the argument type `ARG = std::string` would be deduced,
and the auto-generated specialization of the make unique function
would take the form:

```c++
template <>
std::unique_ptr<std::string> my_make_unique<std::string, std::string>(std::string arg) {    // pass by value
    return std::unique_ptr<std::string>(new std::string(arg));                              // copy ctor
}
```

This would receive its argument `arg` as value, ie. as a copy. So the copy constructor is
called before calling the function. Then it would call the copy ctor of the string for
`new std::string(arg)` - hence the string is copied twice.

Efficiency is also lost for temporary objects:

```c++
int main() {
    std::string s1 = "hello", s2 = "world"
    auto s = my_make_unique<std::string>(s1 + s2);
}
```

The specialization in this case is also the same. The pass-by-value allows the compiler to
initialize the argument with the move ctor, however the function would still call the move
ctor, as "arg" has a name, therefore it is an lvalue. The proper implementation of the
function for an rvalue argument would be

```c++
template <>
std::unique_ptr<std::string> my_make_unique<std::string, std::string>(std::string && arg) {    // pass by ref-ref
    return std::unique_ptr<std::string>(new std::string(std::move(arg)));                      // use move
}
```

Deducing the template argument to a value would also cause problems if the constructor
had an argument taking a variable *by reference*. The call would silently fail, because
the constructor would see the copy in the `make_unique` argument list, and not the original
variable at the call site.

C++ has a language construct to solve this problem. What we need here is called
*perfect argument forwarding*, and it means that

- We should pass the variable with a reference (if the argument is an lvalue)
- We should pass the temporary object with proper rvalue casts (if the argument is an rvalue).

To do this, we write the function like this:

```c++
template <typename T, typename ARG>
std::unique_ptr<T> my_make_unique(ARG && arg) {                 // note ARG &&
    return std::unique_ptr<T>(new T(std::forward<ARG>(arg)));   // note std::forward<ARG>
}
```

So the argument type is `ARG &&`, and we use `std::forward<ARG>` when passing the
argument to the next call.

- The argument of the function takes the form `ARG &&`. Note that is is
not an rvalue reference. *Combining `&&` with templates* creates a new kind
of argument, that is called a *forwarding reference*, or sometimes an *universal
reference*. Universal in the sense that the argument will *automatically*
become `ARG & arg` if an lvalue (variable) is passed at the call,
and it will become `ARG && arg`, when used with an rvalue (temporary object).
It can accept lvalues and rvalues as well.

- At the delegation, we wrap the argument with `std::forward<ARG>`. This function
will do nothing (for lvalues) or it will do an rvalue-cast like `std::move` (for rvalues).
So it knows when it has to behave like `std::move`, and it uses the result of template
argument deduction, that's why it gets `<ARG>`.

(Remark. Actually it works this way. For lvalues, `ARG = U &` is deduced, where `U`
is the type of the argument. In the function header, `U & && arg` is collapsed to
`U & arg`. Forward is instantiated with `<U &>`. For rvalues, `ARG = U` is deduced,
and the function header is `U && arg`, which is able to bind to the temporary.
Forward is then instantiated with `<U>`. So forward gets this piece of information
through its template argument - whether the function argument was an lvalue or an rvalue.)

These are implementation details, nuances you do not really thing about when you
are doing perfect forwarding; just use `ARG &&` as the argument type, and `forward`
for delegation. For any number of arguments, the function takes the form, just
some extra `...`-s where required:

```c++
template <typename T, typename ARG>
std::unique_ptr<T> my_make_unique(ARG && ... arg) {
    return std::unique_ptr<T>(new T(std::forward<ARG>(arg)...));
}
```

## A few words on `auto &&`

The above paragraphs explained how forwarding references worked. Whenever the
compiler found `ARG &&` on the argument list for a templated function
with `typename ARG`, *and* it had to deduce the template argument, it treated
it as a forwarding reference.

The same thing applies to `auto &&`. The keyword `auto` is similar to a template
argument: the compiler deduces the type of a variable using the initializer
This is why `auto &&` is also a forwardng reference – or given its usage contexts,
it's better to call it a universal reference (even if it's not the standardized term).
The most common use is in a range based for:

```c++
for (auto && element : container) {
    // ... do something with the element
}
```

This is better, more general than `auto &` if modification of the elements is
required. For any usual iterator, which returns a reference to an object,
the element will be deduced to be `auto && &`, that is `auto &`, a reference
to the object. But there are some containers, for which the dereferenced iterators
return proxy objects (most notably the `std::vector<bool>` specialization),
and `auto &&` can bind to those proxy objects as well.
