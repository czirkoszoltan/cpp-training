# Function argument manipulation, lambda functions



## std::bind

Consider the following code:

```c++
bool less(int a, int b) {
    return a < b;
}

bool greater(int a, int b) {
    return a > b;
}

bool negative(int a) {
    return a < 0;
}

int main() {
    int arr[] = { 7, -4, 8, -3, -7, 9, 2, 4 };
 
    std::sort(std::begin(arr), std::end(arr), greater);
 
    for (auto i : arr)
        std::cout << i << " ";
    std::cout << std::endl;

    int c = std::count_if(std::begin(arr), std::end(arr), negative);
    std::cout << "Negative: " << c << " elements" << std::endl;
}
```

In this program `greater(a, b)` is the same as `less(b, a)`. Also
`negative(x)` is the same as `less(x, 0)`.

The problem can be generalized: a binary predicate can be used to create an unary predicate, with one of
the arguments fixed. More generalization: from a function of n arguments, you can create a function of m<n
arguments, with the omitted ones bound to a constant value.

Since C++11, `std::bind` can be used to manipulate functions this way. Its first argument is the original
function. Other arguments are the arguments expected by the function. Those can be const (fixed) values, or
can be one of the placeholder symbols `std::placeholders::_1`, `_2` and so on. The order of arguments is
that of the manipulated function. The resulting function will have the the same number of arguments
as that of the placeholder symbols.

With `std::bind`:

```c++
auto negative = std::bind(less, _1, 0);     /* negative(p1) → less(p1, 0) */
auto positive = std::bind(less, 0, _1);     /* positive(p1) → less(0, p1) */
auto greater = std::bind(less, _2, _1);     /* greater(p1, p2) → less(p2, p1) */
```

The type of the return value of `std::bind()` is unspecified, and that is why we use `auto`. It is some
function object. It can be stored in an `std::function<F>` if needed, where F is the call signature
of the stored function, like `bool (int, int)` for `greater`, and `bool (int)` for `negative`.
The `std::function<>` template class will be discussed below in detail.




## std::ref

`std::reference_wrapper`, with this you can pass something by reference, whenever the called function
expects a value.

For a function object:

```c++
class IntPrinter {
  private:
    int count = 0;
  public:
    void operator() (int i) {
        std::cout << "Element " << ++count << " is " << i << std::endl;
    }
};

int main() {
    int arr1[] = { 51, 95, 25, 47 };
    int arr2[] = { 33, 97, 41, 73, 14 };
    IntPrinter p;
    std::for_each(std::begin(arr1), std::end(arr1), std::ref(p));
    // the counter will not restart from 0
    std::for_each(std::begin(arr2), std::end(arr2), std::ref(p));
}
```

Or for bound arguments:

```c++
void incrementer(int &i) {
    ++i;
}

int main() {
    int i = 0;

    // won't work, prints 0
    auto incrementer_bad = std::bind(incrementer, i);
    incrementer_bad();
    std::cout << i;

    // will work
    auto incrementer_good = std::bind(incrementer, std::ref(i));
    incrementer_good();
    std::cout << i;
}
```

The function `std::ref` is just a factory function for an `std::reference_wrapper`.


## Lambda expressions

Predicates, event handlers etc. are nicer in the code, if the function body is at
the call / registration site. Non-local functions just clutter up the code:

```c++
bool greater(int a, int b) {
    return a > b;
}

int main() {
    int arr[] = { 7, 4, 8, 3, 7, 9, 2, 4 };
    std::sort(std::begin(arr), std::end(arr), greater);
}
```

We can implement the function at the call site using a lambda expression:

```c++
int main() {
    int arr[] = { 7, 4, 8, 3, 7, 9, 2, 4 };
    std::sort(std::begin(arr), std::end(arr),
        [](int a, int b) {
            return a > b;
        }
    );
}
```

The lambda expression is just an anonymous function. The syntax is:

- `[]` is the 'lambda introducer', or sometimes it is called the 'capture specifier' (see below).
- `(int a, int b)` the arguments.
- `{ ... }` function body as usual.

The return type can be specified with the arrow syntax (otherwise it is automatically deduced):

```c++
// return a reference (instead of a value)
[] (int & a, int & b) -> int & {
    return a > b ? a : b;
}
```

The bracket can be used to tell which variable can be accessed from the 'outside world',
the containing function:

```c++
std::cin >> center;
std::sort(std::begin(arr), std::end(arr),
    [center] (int a, int b) {
        return std::abs(center-a) < std::abs(center-b);
    }
);
```

This is why it is sometimes called the capture specifier. Possible usages are:

- `[]`: no variables can be used from the containing (outer) function.
- `[x, &y]`: capture x by value, y by reference. x cannot be changed, and beware of lifetime problems with y
- `[=]`: capture all accessed variables by value
- `[&]`: capture all by reference
- `[&, x]`: all by reference, except x, that one by value
- `[=, &x]`: all by value, x by reference
- `[this]`: in a class, capture the `this` pointer. Remember, this is essentially a capture by reference, as `this` is a pointer!
- `[*this]` since C++17: capture the containing object by value.
- `[x = 0]` since C++14: a variable that only exists inside the lambda. The type will be `auto`.

The latter allows one to use `[x = std::move(x)]`, which is the idiomatic way to move an
object into the lambda function object.

The lambda expression essentially creates a function object, which stores the captured values/references
as attributes. Therefore a lambda object can have different size (in bytes) depending on the capture specifier.
All lambda objects' types and sizes are different! You cannot 'name' the type of the lambda object, it is
only a generated type. However, you can use templates, `auto` or an `std::function` wrapper:

```c++
#include <functional>
#include <cmath>
#include <iostream>

class Linear {
    double a_, b_;
  public:
    Linear(double a, double b): a_{a}, b_{b} {}
    double operator() (double x) { return a_*x + b_; }
};
 
int main() {
    // std::function: type erasure for functions
    std::function<double(double)> f;

    // can refer to a free (global) function
    f = sin;
    std::cout << f(1.2) << std::endl;   /* sin(1.2); */

    // or a function object
    f = Linear(1.2, 3.4);
    std::cout << f(6) << std::endl;     /* 1.2*6 + 3.4 */

    // and also a lambda function
    f = [](double x) { return x*x; };
    std::cout << f(3.14) << std::endl;  /* 3.14*3.14 */
}
```

This makes it possible to treat functions as data: store them in variables or containers,
use them in return values etc. Functions become "first class citizens" of the language (almost).

Example:

```c++
#include <functional>
#include <cmath>
#include <iostream>
 
std::function<double(double)> derive(std::function<double(double)> f) {
    double dx = 0.001;
    auto derived = [=] (double x) {
        return (f(x+dx) - f(x)) / dx;
    };
 
    return derived;
}
 
int main() {
    auto my_cos = derive(sin);
 
    for (double x = 0; x < 3.14; x += 0.1)
        std::cout << cos(x) - my_cos(x) << std::endl;
}
```

Remark. With a C++14 compiler, you can also use `auto` to write the derive function:

```c++
template <typename FUNC>
auto derive(FUNC f) {
    double dx = 0.001;
    auto derived = [=] (double x) {
        return (f(x+dx) - f(x)) / dx;
    };
 
    return derived;
}
```

But this is a special case, eg. you cannot write `std::vector<auto>` to store function
objects, you must use `std::vector<std::function<double(double)>>`.

Processing the click of a button in some GUI environment.

```c++
class Button {
  private:
    std::string label;
    std::function<void()> on_click_callback;
  
  public:
    void on_click(std::function<void()> f) {
        on_click_callback = std::move(f);
    }
    void process_click() {
        if (on_click_callback)
            on_click_callback();
    }
};


Button b;
b.on_click([]{
    std::cout << "hello\n";
});

/* ... */

b.process_click();
```
