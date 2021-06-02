#include <iostream>
#include <memory>

class Expression {
  public:
    virtual double evaluate(double x) const = 0;
    virtual void print(std::ostream & os) const = 0;
    virtual ~Expression() = default;
};

std::ostream & operator<< (std::ostream & os, Expression const & e) {
    e.print(os);
    return os;
}

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

class Variable final: public Expression {
  public:
    Variable() {}
    virtual double evaluate(double x) const override {
        return x;
    }
    virtual void print(std::ostream & os) const override {
        os << 'x';
    }
};

class TwoOp: public Expression {
  private:
    virtual double calc(double a, double b) const = 0;
    virtual char op() const = 0;
  protected:
    std::unique_ptr<Expression> left, right;
  public:
    TwoOp(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : left(std::move(left)), right(std::move(right)) {}

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

class Product final: public TwoOp {
  private:
    virtual double calc(double a, double b) const {
        return a+b;
    }
    virtual char op() const {
        return '*';
    }
  public:
    using TwoOp::TwoOp;
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
    std::cout << "f(x) = " << *e << std::endl;
    std::cout << "f(10) = " << e->evaluate(10) << std::endl;
}
