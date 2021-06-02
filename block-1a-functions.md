# C++ functions

## Argument passing

By default, all arguments are passed as value, like in C:

```c++
void increment(int x) {
    x += 1;
}

int a = 3;
increment(a); // nothing happens, as x will store a copy of the value in a, and not a itself
```

In order to change to value, pass the variable by reference, using Type &:

```c++
void increment(int &x) {
    x += 1;
}

int a = 3;
increment(a); // value of a becomes 4
```

This is works for all objects and structs (which are the same in C++):

```c++
class Rational {
  public:
    unsigned num, den;
}

// Reference to a variable of type Rational
void increment(Rational &r) {
    r.num += r.den;
}
```

Possibilities:

```c++
// Pass a copy of the value (for built-in types and working copies)
void func(Type obj);

// Pass the reference of the variable, out parameter of the function
void func(Type & obj);

// By reference, but must not change. No copying takes place (eg. for large objects)
void func(Type const & obj)

// Reference of a temporary object, we'll talk about this later in detail
// void func(Type && obj)
```

What's the difference between the first (by value) and the third (by const reference) one?
For the caller: nothing, as the variable passed will not change. So it is only important
for the callee. The callee decides if a working copy is needed. For example:

```c++
std::string space_to_underscore(std::string s) {
    for (size_t i = 0; i < s.length(); ++i)
        if (s[i] == ' ')
            s[i] = '_';
    return s;
}
```

Here we would make a copy anyway, so it is easier (and more effective, see later) to 
get the argument as an object (and not a reference). In modern C++, we express this
with a range-based for loop (a thing also discussed later):

```c++
std::string space_to_underscore(std::string s) {
    for (char & c : s)
        if (c == ' ')
            c = '_';
    return s;
}
```


References in other contexts:

```c++
int x = 3;
// y and x are different names for the same integer in memory
int &y = x;
```

Beware:

```c++
int &func() {
    int x = 3;
    return x; // bug: x is deleted from memory
}

// "Dangling reference", bound to a non-existiing variable.
// Essentially the same as a dangling pointer.
// Sometimes the compiler will catch it, otherwise very hard to debug.
int &r = func();
```
