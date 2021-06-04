# C++ classes

## Class definitions

```c++
#include <iostream>

class Rational {
  private:
    unsigned num, den;
    static unsigned euclid(unsigned a, unsigned b) {
        while (b != 0) {
            unsigned t = b;
            b = a%b;
            a = t;
        }
        return a;
    }
  public:
    Rational(unsigned num_, unsigned den_ = 1) {
        unsigned gcd = euclid(num_, den_);
        num = num_/gcd;
        den = den_/gcd;
    }
    unsigned get_num() const { return num; }
    unsigned get_den() const { return den; }
    Rational operator+(Rational rhs) const;
    explicit operator double () const {
        return (double)num / (double) den;
    }
};

// Non-inline implementation, classname::functionname as the name of the function.
// Operator overload; r1+r2 expression will call r1.operator+(r2) method
Rational Rational::operator+(Rational rhs) const {
    return Rational(num * rhs.den + rhs.num * den,
                    den * rhs.den);
}

// Operator overload; r1*r2 expression will call operator*(r1, r2) free function.
// Use whichever leads to nicer design.
// As this is not a method (member function), this must use the public interface.
Rational operator*(Rational lhs, Rational rhs) {
    return Rational(lhs.get_num() * rhs.get_num(),
                    lhs.get_den() * rhs.get_den());
}

// *= won't be generated automatically, even when there is *.
// Left hand side argument must be passed as reference,
// as the expression x += y changes the value of x.
// Returns the left hand side variable itself,
// therefore the return type is also a reference.
Rational & operator*=(Rational &lhs, Rational rhs) {
    lhs = lhs * rhs;
    return lhs;
}

// Print: std::cout << 1 << 2 << 3 means ((std::cout << 1) << 2) << 3,
// every printing operator returns std::cout, the stream.
std::ostream & operator<<(std::ostream & os, Rational r) {
    os << r.get_num() << '/' << r.get_den();
    return os;
}

int main() {
    // Objects as local variables, just as in C
    Rational a(1, 3);
    Rational b(1, 2);
    std::cout << a+b << std::endl;
    // Temporal object
    std::cout << Rational(5, 10) << std::endl;
}
```

Remark: in C++ `class` and `struct` are the same. For classes, default visibility is
private, and for structs, it is public. We usually prefer `struct` for data transfer objects,
and `class` for classes with behaviorm whenever there are methods.


## Conversions

Constructors with one argument, and conversion operators are used automatically
by the compiler. For the previous example:

```c++
class Rational {
    // unsigned -> Rational conversion
    Rational(unsigned num_, unsigned den_ = 1);
    // Rational -> double conversion
    explicit operator double () const;
};

// 3/1, because Rational a = Rational(3);
Rational a = 3;
Rational b(4, 5);
// b + Rational(6), quite convenient!
std::cout << b + 6;

// Will print 0.8
std::cout << (double) b;
```

Conversions can be implicit (automatic) or explicit (requested by
the programmer). The `explicit` keyword will disable automatic conversion.
This is important, and it should be the default:

```c++
class Rational {
    // unsigned->Rational konverzió, can be automatic
    Rational(unsigned num_, unsigned den_ = 1);
    // Rational->double, can result in data loss, should be explicit
    explicit operator double () const;
};

class MyArray {
    // This is NOT an integer->MyArray conversion, should be explicit
    explicit MyArray(unsigned size);
};
```

Technically this is also a conversion:

```c++
int i;
while (std::cin >> i) {     // std::cin.operator bool()
    /* ... */
}
```

## C++ classes and resource management

Motivation: strings are char arrays, but arrays are hard to
handle, pass between functions, manage their storage. A simple
string concatenation function:

```c++
char *concat_strings(char const *first, char const *second) {
    size_t newlen = strlen(first) + strlen(second);
    char *concat = new char[newlen + 1]; // +1 for \0
    strcpy(concat, first);
    strcat(concat, second);
    // return pointer to array, note there is no delete[] here
    return concat;
}

char *stuff = concat_string("apple", "tree");
std::cout << stuff;
// must not forget this line
delete[] stuff;
```

This should look like this

```c++
String s1 = "apple", s2 = "tree";
String stuff = s1+s2;
```

Implementation of a string class is below. See the comments!

```c++
#include <iostream>
#include <stdexcept>
#include <cstring>  // low level char array functions

class String {
  private:
    char *data;     // has \0 terminator
    size_t len;     // real length without the \0
    explicit String(size_t initlen);

  public:
    String(char const *init);
    ~String();
    String(String const &s);
    String & operator=(String const &s);
    char & operator[](size_t idx);
    char const & operator[](size_t idx) const;
    String operator+(String const &rhs) const;
    String& operator+=(String const &rhs);
    explicit operator char const * () const;
};

// Constructor: allocates memory and copies string
String::String(char const *init) {
    len = strlen(init);
    data = new char[len+1];
    strcpy(data, init);
}

// Destructor: if the string object is destroyed,
// the accompanying character array must be deleted as well.
// The dtor is called automatically at the end of the lifetime:
// - local variables at the end of blocks
// - global variables at the end of the process
// - calling delete on dynamically allocated objects
// - end of the lifetime of a container object
String::~String() {
    delete[] data;
}

// Copy ctor. It is called automatically for:
//   1) explicit copying: String s2 = s1; or String s2(s1);
//   2) Passing function arguments by value: void func(String s);
//   3) Return value of function: String func();
//   4) Throwing exceptions, as part of an exception object
// Rule of three: if there is a destructor, usually a copy ctor
// and an operator= is required as well.
String::String(String const &s) {
    len = s.len;
    data = new char[len+1]; // most important line
    strcpy(data, s.data);
}

// Assignment operator: see the copy ctor and the dtor
String & String::operator=(String const &s) {
    if (this != &s) {   // Avoid problems of self assignment
        delete[] data;
        len = s.len;
        data = new char[len+1];
        strcpy(data, s.data);
    }
    return *this;   // For usual operator= semantics, like a = b = c;
}

// Also handles out of bound indexing
char & String::operator[] (size_t idx) {
    if (idx >= len)
        throw std::out_of_range("string index out of range");
    return data[idx];
}

// Const string -> const character.
// (Note that the array internally is not const.)
char const & String::operator[] (size_t idx) const {
    if (idx >= len)
        throw std::out_of_range("string index out of range");
    return data[idx];
}

// Private constructor for the concatenation operations.
// Allocates the char array, but it is uninitialized.
String::String(size_t initlen) {
    len = initlen;
    data = new char[len+1];
}

// The array is created with the private ctor, 
// and then initialized.
String String::operator+(String const &rhs) const {
    String concat(len + rhs.len);
    strcpy(concat.data, this->data);
    strcat(concat.data, rhs.data);
    return concat;
}

// Boilerplate code, we have to create this manually.
String& String::operator+=(String const &rhs) {
    *this = *this + rhs;
    return *this;
}

String::operator char const * () const {
    return data; // valid as C string, as it has \0
}

std::ostream & operator<< (std::ostream & os, String const & s) {
    os << (char const *) s;
    return os;
}

int main() {
    String s1 = "apple";
    String s2 = "tree";
    String s3 = s1+s2;
    std::cout << s3 << std::endl;
    try {
        s3[100] = 'X';
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
}
```

Remarks:

- Check the `const`s in the code above, figure out where
they are needed and why.

- Also think about `explicit`s.

- "The copy constructor is called for argument passing
and return values" - this is not necessarily true.

Example 1:

```c++
void myfunc(String s) {
    // do something with the string...
}

int main() {
    myfunc("appletree");
}
```

`myfunc(String s)` says "I need a String object
as an argument". The call site has a char array (converted
to a pointer by taking its address).  Therefore the
`String(char const *)` constructor will be called.

Example 2:

```c++
String otherfunc() {
    return "appletree",
}
```

This will work the same way. In order to return from the
function, a string object must be created. The char array
can be used to initialize the objects, therefore
`String(char const *)` is called. That is the constructor
that can accept this type.

Example 3:

```c++
void yetanotherfunc(String s) {
    // ...
}

int main() {
    String s1 = "apple", s2 = "tree";
    yetanotherfunc(s1 + s2);
}
```

The concatenation operator creates a string object containing
"appletree". This object is an unnamed temporary object, not
a variable. To pass the object, we might make a copy of it
using the copy ctor - however it is also acceptable to pass
the temporary object itself, without copying. This is called
copy elision, and is a great optimization technique, used extensively
by compilers. More on that later.

## std::string_view

Consider the following functions, both taking a string as argument:

```c++
void delete_file_1(std::string const & filename);
void delete_file_2(char const * filename);
```

Both versions have their own problems.

- The `std::string` version is nice if you have an `std::string`
object, and you want to pass it to the function. The reference
is bound to that object. However when you call it like
`delete_file_1("example.txt")`, then the argument is a character
array, and a string object is created with the `string(char const *)`
constructor. This requires memory allocation, array copying etc.
so it is not efficient. The copy of the array is unnecessary.

- The `char const *` version works nicely for the `delete_file_2("example.txt)"`
case; the pointer argument will point to the character array, and that's all.
However for the `std::string` object, you will have to call it as
`delete_file_2(fn.c_str())`, to "convert" it to a `char const *`.
(`std::string` has no `char const *` cast operator, to avoid ambiguity.)

This is why C++17 introduced `std::string_view`. This class represents
a string, *without any memory management*. Hence the name `view`. It
has conversions from `char const *` and `std::string`; if you have your
own string class (please don't), you can write a conversion operator for it
to create an `std::string_view`.

```c++
void delete_file_3(std::string_view filename);

std::string fn = "example.txt";
delete_file_3(fn);

delete_file_3("example.txt");
```

Neither call requires boilerplate code. Also neither call will do
any memory management.

## C++ exceptions

Always throw exceptions as value, and catch
them by reference. This makes memory management
of exception objects automatic, and allows the
catch to group errors by base classes.
See the string example.
