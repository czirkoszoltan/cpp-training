#include <iostream>
#include <memory>

class Expression {
  public:
    virtual double evaluate(double x) const = 0;
    virtual void print(std::ostream & os) const = 0;
    virtual std::shared_ptr<Expression> derivative() const = 0;
    virtual ~Expression() = default;
};

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
    virtual std::shared_ptr<Expression> derivative() const {
        return std::make_shared<Constant>(0);
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
    virtual std::shared_ptr<Expression> derivative() const {
        return std::make_shared<Constant>(1);
    }
};

class TwoOp: public Expression {
  private:
    virtual double calc(double a, double b) const = 0;
    virtual char op() const = 0;
  protected:
    std::shared_ptr<Expression> left, right;
  public:
    TwoOp(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right)
        : left(left), right(right) {}

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
    virtual std::shared_ptr<Expression> derivative() const {
        return std::make_shared<Sum>(left->derivative(), right->derivative());
    }
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
    
    virtual std::shared_ptr<Expression> derivative() const {
        return std::make_shared<Sum>(
            std::make_shared<Product>(left, right->derivative()),
            std::make_shared<Product>(left->derivative(), right)
        );
    }
};

int main() {
    // 4 * (5+x)
    auto e = std::make_shared<Product>(
        std::make_shared<Constant>(4),
        std::make_shared<Sum>(
            std::make_shared<Constant>(5),
            std::make_shared<Variable>()
        )
    );
    e->print(std::cout);
    std::cout << '=';
    std::cout << e->evaluate(10) << std::endl;
    
    auto ed = e->derivative();
    ed->print(std::cout);
}
