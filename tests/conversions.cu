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
    operator int() const {
        return m_val;
    }
};

template<typename S0, typename S1>
struct flat {
  typedef S0 T0;
  typedef S1 T1;
};

template<typename Flat>
struct checker {
  static void impl(const typename Flat::T0& i) {
    std::cout << "T0" << std::endl;
  }
  static void impl(const typename Flat::T1& i) {
    std::cout << "T1" << std::endl;
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
    x = converts_to_int(10);
    variant::apply_visitor(reflect, x);
    
    std::cout << std::endl << "True result: ";
    reflect(10.0);
    reflect(10.0f);
    reflect(10);
    reflect(true);
    reflect(10);
    reflect(10);
    std::cout << std::endl;

}
