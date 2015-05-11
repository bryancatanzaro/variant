#include <variant/variant.h>
#include <iostream>
#include <typeinfo>

struct reflector {
    template<typename T>
    void operator()(const T&) {
        std::cout << typeid(T).name();
    }
};

template<typename T>
struct converts_to_T {
    T m_val;
    converts_to_T(T val) : m_val(val) {}
    operator T() const {
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
    x = converts_to_T<int>(10);
    variant::apply_visitor(reflect, x);
    x = converts_to_T<float>(10.0);
    variant::apply_visitor(reflect, x);
    x = converts_to_T<bool>(true);
    variant::apply_visitor(reflect, x);
    
    std::cout << std::endl << "True result: ";
    reflect(10.0);
    reflect(10.0f);
    reflect(10);
    reflect(true);
    reflect(10);
    reflect(10);
    reflect(10.0f);
    reflect(true);
    
    std::cout << std::endl;

}
