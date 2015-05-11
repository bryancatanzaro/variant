#include <variant/variant.h>
#include <iostream>

struct foo{};
struct bar{};

typedef variant::variant<foo, bar> variant_0;
typedef variant::variant<variant_0, int> variant_1;

struct variant_0_printer {
    void operator()(const foo& x) const {
        std::cout << "foo";
    }
    void operator()(const bar& x) const {
        std::cout << "bar";
    }
};

struct variant_1_printer {
    void operator()(const variant_0& x) const {
        std::cout << "variant_0: ";
        variant::apply_visitor(variant_0_printer(), x);
    }
    void operator()(const int& x) const {
        std::cout << "int: " << x;
    }

};



int main() {
    variant_1 x(10);
    variant::apply_visitor(variant_1_printer(), x); std::cout << std::endl;
    x = foo();
    variant::apply_visitor(variant_1_printer(), x); std::cout << std::endl;
    x = bar();
    variant::apply_visitor(variant_1_printer(), x); std::cout << std::endl;

    std::cout << "Size of variant<foo, bar>: ";
    std::cout << sizeof(variant_0) << std::endl;
    std::cout << "Size of variant<variant<foo, bar>, int>: ";
    std::cout << sizeof(variant_1) << std::endl;
}
