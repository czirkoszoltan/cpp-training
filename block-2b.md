# C++: class hierarchies, smart pointers

## Example: expressions

Let's build a class hierarchy to store mathematical functions with one argument.

- Expression: base class (interface)
- Constant, variable, sum, product.

![Absztrakt szintaxisfa](asset/2b-expr.svg)

The base class:

```c++
class Expression {
  public:
    virtual double evaluate(double x) const = 0;
    virtual void print(std::ostream & os) const = 0;
    virtual ~Expression() = default;
};
```

- Functions which should be overriden in derived classes must be virtual. Non-virtual functions cannot be overriden in derived classes, but are faster. (Virtual functions are usually implemented internally with pointers to functions.)
- `=0` in the declaration: pure virtual function, which makes the class abstract. There is no separate keyword.
- C++ does not tell apart abstract classes and interface, like other OOP languages. Still we usually call classes like the above interfaces.
- The destructor is also virtual, see below. But it can and should be implemented by the compiler, hence the `= default`.

Rule of thumb: whenever there is at least one virtual function, there should be a virtual destructor as well. In other words, if a class is intended to be used as a base class in a hierarchy, the destructor should be virtual, even if it does not seem to serve any purpose. The derived classes might do have some cleanup tasks to do!

  

## Implementation of the expression classes

The constant is trivial:

```c++
class Constant final: public Expression {
  private:
    double c;
  public:
    Constant(double c) : c(c) {}
    virtual double evaluate(double) const override {
        return c;
    }
    virtual void print(std::ostream & os) const override {
        os << c;
    }
};
```

- `final`: cannot be derived from.
- `override`: the base class must have a function like this, otherwise compiler error. This can catch silly coding mistakes like omitting the const keyword, and also helps when refactoring class hierarchies.

Here the virtual function calls can be demonstrated:

```c++
Expression *e = new Constant(5.6);
e->print(std::cout);
std::cout << e->evaluate(10);
delete e;
```

- The expression `new Constant` has a value `Constant *`, which is stored in the variable with type `Expression *`.
- Still `e->print` and `e->evaluate` will call `Constant` methods,
  as the object stores a pointer to the functions, ie. knows its own type.

The variable:

```c++
class Variable final: public Expression {
  public:
    virtual double evaluate(double x) const override {
        return x;
    }
    virtual void print(std::ostream & os) const override {
        os << 'x';
    }
};
```

The `Sum` class is a little longer:

```c++
class Sum final: public Expression {
  private:
    Expression *left, *right;
  public:
    Sum(Expression *left, Expression *right) : left(left), right(right) {}
    Sum(Sum const &) = delete;
    Sum & operator=(Sum const &) = delete;
    ~Sum() {
        delete left;
        delete right;
    }
    virtual double evaluate(double x) const override {
        return left->evaluate(x) + right->evaluate(x);
    }
    virtual void print(std::ostream & os) const override {
        os << '(';
        left->print(os);
        os << '+';
        right->print(os);
        os << ')';
    }
};

int main() {
    Expression *e = new Sum(new Variable, new Constant(5));
    std::cout << e->evaluate(10);
    delete e;
}
```

- The object has two children, which can be any kind of expressions (polymorphic).
- So it cannot store these by value, because each subtype of the expression class has different size (`Variable` has no member, `Constant` has one member of type `double` etc.)
- An indirection solves the problem: the object will store a *pointer* to each children.
- Unfortunately the lifetime and memory management of the container`Sum` and its two children `Expression` is decoupled. When a `Sum` is destroyed, only the contained pointers are destroyed, but not the subtrees. We have to do the memory management by hand: allocate the subtrees from the free store with new, and delete them in the destructor.
- Here we can see the importance of the virtual destructor in the base class. `delete left` and `delete right` call the destructors of the subobjects, but the static type of the pointers is `Expression*`. Yet, the proper destructor must be called.
- Remember, classes implementing their own destructor should have copy constructors and assignment operators. We do not implement those now, however we use `= delete` to prevent accidental call of the functions.

The `Product` class is almost the same.



## A base class for Sum and Product

The common parts of`Sum` and `Product` (`left`, `right`, constructors) can be refactored to a base class:

```c++
class TwoOp: public Expression {
  protected:
    Expression *left, *right;
  public:
    TwoOp(Expression *left, Expression *right) : left(left), right(right) {}
    TwoOp(TwoOp const &) = delete;
    TwoOp & operator=(TwoOp const &) = delete;
    ~TwoOp() {
        delete left;
        delete right;
    }
};

class Sum final: public TwoOp {
  public:
    using TwoOp::TwoOp;
    
    virtual double evaluate(double x) const override {
        return left->evaluate(x) + right->evaluate(x);
    }
    virtual void print(std::ostream & os) const override {
        os << '(';
        left->print(os);
        os << '+';
        right->print(os);
        os << ')';
    }
};

class Product final: public TwoOp {
  public:
    using TwoOp::TwoOp;

    virtual double evaluate(double x) const override {
        return left->evaluate(x) * right->evaluate(x);
    }
    virtual void print(std::ostream & os) const override {
        left->print(os);
        os << '*';
        right->print(os);
    }
};
```

- Constructors are not inherited automatically, hence the `using TwoOp::TwoOp` code line. It means the following: treat the code as if the `TwoOp::TwoOp` method (ie. the constructor of the `TwoOp` class) was declared here.

By creating two more virtual methods, the evaluator and the printer methods can also be finalized:

```c++
class TwoOp: public Expression {
  private:
    virtual double calc(double a, double b) const = 0;
    virtual char op() const = 0;
    /* ... */
  public:
    virtual double evaluate(double x) const override final {
        return calc(left->evaluate(x), right->evaluate(x));
    }
    virtual void print(std::ostream & os) const override final {
        os << '(';
        left->print(os);
        os << op();
        right->print(os);
        os << ')';
    }
};

class Sum final: public TwoOp {
  private:
    virtual double calc(double a, double b) const {
        return a+b;
    }
    virtual char op() const {
        return '+';
    }
  public:
    using TwoOp::TwoOp;
};
```

- Note that in C++ `private` virtual functions can be overridden in derived classes. `private` only refers to the callers (ie. who can call the function) and not the implementers (who can implement/override the function).
- A private pure virtual method is self-documenting: if you derive from this class, you must implement this method; however you have nothing else to do, as you cannot invoke it.




## TwoOp is not a class for memory management: use std::unique_ptr

The `TwoOp` class implements business logic, and it has nothing to do with memory management. Using `delete` here violates an OOP principle, the single responsibility principle. This is the same reason why we also implemented a string class: to get rid of manual memory management, and hide it inside the string. Is it possible to get rid of these `delete` calls?

The answer is: yes, using smart pointers. A smart pointer is a class, which acts like a raw pointer, but also does some memory management.

```c++
template <typename T>
class UniquePtr {
  private:
    // internally there is a raw pointer
    T *ptr;

  public:
    // from the outside it looks and acts like a pointer
    T& operator*() const {
        return *ptr;
    }
    // however when destroyed, it also destroys the objects it is responsible for
    ~UniquePtr() {
        delete ptr;
    }
};
```

The standard `std::unique_ptr<T>` implements the same idea. The name `unique_ptr` refers to the fact that it is the *only* pointer which stores the address of the objects. That's why it can destroy the object when it is destroyed:

```c++
{
    // allocate an integer and give it the the unique_ptr p
    std::unique_ptr<int> p(new int);
    *p = 3;
}
// there is no delete call here, as the destructor of p has taken care of that
```

```c++
{
    std::unique_ptr<int> p(new int);
    std::unique_ptr<int> p2 = p;        // compile error, p2 would also point to the same integer
}
```

```c++
{
    std::unique_ptr<int> p(new int);
    // move: we hand over the integer to p2, and p becomes a null pointer
    std::unique_ptr<int> p2 = std::move(p);
}
// p2 destructor will delete the integer
```

```c++
{
    // make_unique helper function: allocate an object from the free store,
    // and wrap it in a unique_ptr.
    auto p = std::make_unique<int>();
}
```

The important part of `TwoOp`, with the `4*(5+x)` expression created in the `main` function:

```c++
class TwoOp: public Expression {
  private:
    std::unique_ptr<Expression> left, right;
  public:
    TwoOp(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : left(std::move(left)), right(std::move(right)) {}
    // no destructor, it is generated automatically
    // copy ctor is deleted automatically, as the copy ctor of unique_ptr is deleted
    // ...
};

int main() {
    // 4 * (5+x)
    auto e = std::make_unique<Product>(
        std::make_unique<Constant>(4),
        std::make_unique<Sum>(
            std::make_unique<Constant>(5),
            std::make_unique<Variable>()
        )
    );
    e->print(std::cout);
    std::cout << '=';
    std::cout << e->evaluate(10);
    // no delete call here
}
```




## Deriving expressions, std::shared_ptr

Symbolic differentiation for our expressions:

| Type | Differentiated |
| ---- | -------------- |
| c    | 0              |
| x    | 1              |
| a+b  | a'+b'          |
| a×b  | a'×b + a×b'    |

When implementing this, we run into a problem. The two inner products store the differentiated left and right hand side, but the original expressions as well. So the new products *and* the original expressions also use those subtrees. That will be a compile error in our code, as it would require copying the unique_ptr.

```c++
class Product final: public TwoOp {
    /* ... */
  public:
    std::unique_ptr<Expression> derivative() const {
        return std::make_unique<Sum>(
            std::make_unique<Product>(left->derivative(), right),  // compile error
            std::make_unique<Product>(left, right->derivative())
        );
    }
};
```

Two possible solutions:

- Deep copying the subtrees (and not just the pointers).
- Making common subtrees in expressions possible.

We choose the latter one to demonstrate `std::shared_ptr`. It works exactly like `unique_ptr`, however allows *more pointers* sharing the same object – hence its name.

`shared_ptr` uses reference counting internally. A group of `shared_ptr`'s pointing to the same object is called a sharing group. When the sharing group becomes empty, ie. the reference counter becomes zero, the managed object is `delete`-d.

```c++
{
    auto p = std::make_shared<int>();
    *p = 3;

    // p2 and p point to the same integer
    auto p2 = p;

    // p becomes null, but the integer remains in memory, p2 still points to it
    p = nullptr;
    *p2 = 4;
}
// now the integer is deleted, because p2 was also destroyed
```

This makes it easy to implement the product derivative

```c++
class Product final: public TwoOp {
  public:
    virtual std::shared_ptr<Expression> derivative() const {  // return type could be auto
        return std::make_shared<Sum>(
            std::make_shared<Product>(left, right->derivative()),
            std::make_shared<Product>(left->derivative(), right)
        );
    }
};
```


## Downloads

- [Expressions with `unique_ptr`](asset/2b-expr-unique.cpp)
- [Expressions with `shared_ptr` and differentiation](asset/2b-expr-shared.cpp)
