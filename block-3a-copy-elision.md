# Argument passing, copy elision

## In theory

Execution of a function call:

- The caller allocates space on the stack for the return value. The object is not yet created(!).
- The caller pushes arguments on the stack.
- The function is called; the CPU pushes the return address on the stack.

Returning from the call:

- The callee initializes the return value (creates the object at the space allocated by the caller for this purpose).
- Return from the call; the CPU pops the return address from the stack.
- The caller destroys the arguments, and it can work with the returned object.

The first example demonstrates this. First the integer 6 is pushed to the stack, and also the constructor of `std::string` is called, because the argument is passed by value:

```c++
void func(int x, std::string s) {
    /* ... */
}

std::string h = "hello world";
func(6, h);
```

The second example shows a return value. On the call, space is allocated for a `string` object, but it is not yet initialized (as the caller can't know, what its contents will be). When the `return` statement is executed, the constructor is called. After returning from the function, the printing takes place, and finally the temporary string object is destroyed at the end of the line. (The precise term is: after evaluating the full expression, but that is usually the semicolon.)

```c++
std::string func() {
    return "hello world";   // calls std::string(char const *)
}

std::cout << func();
```

How do local variables work? Those are destroyed when exiting the scope. In the example below, `a` and `b` may be destroyed when the function returns. The `return` statement initializes the return object; the value it is used for this purpose is object `a`. As it is a string, the copy constructor will be called (that can accept the string object as an argument):

```c++
std::string func() {
    std::string a = "apple";
    std::string b = "banana";
    return a;
}
```

## In practice: the copy elision technique

With the stack based memory management discussed previously, each object – arguments as well as return values – have a predefined location on the stack. So often the compiler is forced to call the copy constructor, followed by a destructor call, in order to *move* and object around. However, this copying is unnecessary, we don't need *two* objects: the purpose of the copy+destroy sequence in this case is moving the object.

That is why the C++ standard allows the compilers to optimize copy+destruct pairs away; even if the copy constructor or the destructor of a class is implemented by the programmer, and *even if they have side effects*. That means that a printing `std::cout << "hello";` line might be optimized away as well. (This is necessary because `new` and `delete` ie. free store memory management is technically also a side effect.)

Some examples below. In the next function the variable `a` should be on the stack, among other variables. However, the compiler will detect that the value of this local variable is the return value itself (`return a`), therefore it will place this variable at the location of the return value. This way then `return` itself is a no-op; the string is already at the correct location. This is called NRVO, named return value optimization:

```c++
std::string func() {
    std::string a = "apple";  // will be placed at the location of the return value
    return a;  // usually a no-op, zero cost
}
```

This also works for some expressions. Temporary objects are destroyed at the end of theline, however in the next function it can be placed where the caller expects the return value, and then the `return` statement is a no-op. This is URVO, unnamed return value optimization:

```c++
std::string func() {
    return std::string(5, 'o');  // "ooooo"
                                 // no copying, the return is initialized directly
}
```

When a function returns an object, we can extend its lifetime by storing it in a variable. In the next example, the expression `a+b` creates the concatenated string, which would be a temporary object. However, we initialize the variable `c` with this newly created string. The compiler might copy the contents of the temporary object to `c`, then destroy the temporary. However, it is more efficient to keep the temporary and give it the name `c`.

```c++
std::string a = "apple", b = "tree";
std::string c = a + b;   // extend the lifetime of the temporary
```

This also works when passing arguments by value. The function below expects a newly created string object as an argument.
After calling `a+b`, we have a temporary. It might be copied and then the temporary destroyed, however with clever usage
of the stack, the compiler is able to organize objects so that the concatenated string object is created where `func`
expects its first argument `s`. This way, passing by value is a no-op, with zero runtime cost. Note that this is possible
because the temporary has no name; we have no means to access that object.

```c++
void func(std::string s);

std::string a = "apple", b = "tree";
func(a + b);   // pass the temporary itself
```

Since C++17, the compiler is actually required to be able to do such optimizations (guaranteed copy elision).


## Calling conventions

All operating systems and architectures have rules, how function calls work: where to place the arguments, and in what order; where to place the return value; how a function can tell the caller, that it throws an exception. These rules are the calling conventions. These are defined in the ABIs (application binary interface).

As function calls are quite common, the rules are made to be fast. Arguments are usually passed in CPU registers, whenever possible. Using the stack is actually the last resort. 

Typical examples:

```c++
int prod(int a, int b) {
    return a * b;
}
```

Compilation with optimization results in the following instructions. The calling convention is: 1) the first argument that has the size of 4 bytes go in register `edi`, 2) the secondone to `esi`, 3) the return value goes into `eax` if it fits (4 bytes):

```asm
prod(int, int):
  mov eax, edi      ; eax = edi
  imul eax, esi     ; eax *= esi
  ret               ; return eax
```

If we turn off optimization, something strange happens. It is not the caller, but the callee(!!!) is storing the arguments into the memory. The reason behind this is whether or not the code is optimized, it most follow the ABI rules (which state that these arguments will be received in registers). However, with optimization turned off, the programmer wants to debug the code, and see the argumentsin the memory – so they must be stored:

```asm
prod(int, int):
  push rbp
  mov rbp, rsp
  mov DWORD PTR [rbp-4], edi
  mov DWORD PTR [rbp-8], esi
  mov eax, DWORD PTR [rbp-4]
  imul eax, DWORD PTR [rbp-8]
  pop rbp
  ret
```

A surprising example:

```c++
double myfunc(double x) {
    return sin(x);
}
```

As the `sin` function has the `double(double)` call signature as well, when we arrive inside this function, the argument is at the location where `sin` would expect it to be. Also `sin` would place the return value to the place where `myfunc` should. Therefore we can just jump to the implementation of `sin`:

```asm
myfunc(double):
  jmp sin
```

The online compiler at https://gcc.godbolt.org is a nice tool to make experiments with code snippets like this.
Be sure to check all code with the `-O0` (unoptimized) and with `-O2` (optimized) flag as well.
