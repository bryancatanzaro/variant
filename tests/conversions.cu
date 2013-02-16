#include <variant/variant.h>
#include <iostream>
#include <typeinfo>

struct reflector : public variant::static_visitor<> {
    template<typename T>
    void operator()(const T&) {
        std::cout << typeid(T).name();
    }
};

struct converts_to_int {
    int m_val;
    converts_to_int(int val) : m_val(val) {}
    operator int() {
        return m_val;
    }
};


int main() {
    reflector reflect;

    typedef variant::variant<bool, int, float, double> my_variant;

    std::cout << "Test result: ";
    
    my_variant x = 10.0;
    variant::apply_visitor(reflect, x);
    x = 10.0f;
    variant::apply_visitor(reflect, x);
    x = 10;
    variant::apply_visitor(reflect, x);
    x = true;
    variant::apply_visitor(reflect, x);
    x = (short)10;
    variant::apply_visitor(reflect, x);
 
    std::cout << std::endl << "True result: ";
    reflect(10.0);
    reflect(10.0f);
    reflect(10);
    reflect(true);
    reflect(10);
    std::cout << std::endl;
}
