#include "variant.h"
#include <iostream>
#include <thrust/copy.h>
#include <thrust/device_vector.h>

//Some data types
struct foo{
    int o;
    //The first type in a variant must have a default constructor
    __host__ __device__
    foo() : o(0) {};
    
    __host__ __device__
    foo(int _o) : o(_o) {};
};
struct bar{
    int r;
    __host__ __device__
    bar(int _r) : r(_r) {};
};
struct baz{
    int z;
    __host__ __device__
    baz(int _z) : z(_z) {};

};

//A variant
typedef variant::variant<foo, bar, baz> my_variant;

//A visitor
struct my_visitor : public variant::static_visitor<int> {
    __host__ __device__
    int operator()(const foo& p) const {
        return 10 + p.o;
    }

    __host__ __device__
    int operator()(const bar& p) const {
        return 20 + p.r;
    }

    __host__ __device__
    int operator()(const baz& p) const {
        return 30 + p.z;
    }
};

//A functor that applies the visitor
struct extractor {
    typedef int result_type;
    __host__ __device__
    int operator()(const my_variant& v) const {
        return variant::apply_visitor(my_visitor(), v);
    }
};
        

//We can properly deal with variants containing convertible objects
typedef variant::variant<float, double, int, long> convertibles;

#include <typeinfo>
struct reflector : public variant::static_visitor<> {
    template<typename T>
    void operator()(const T&) const {
        std::cout << typeid(T).name();
    }
};


int main() {
    //Make a vector of variants on the GPU
    thrust::device_vector<my_variant> x(3);

    foo a(1);
    bar b(2);
    baz c(3);

    //Initialize their elements
    x[0] = c;
    x[1] = a;
    x[2] = b;
    thrust::device_vector<int> y(3);
    //Visit the vector on the GPU
    thrust::transform(x.begin(), x.end(), y.begin(), extractor());
    std::ostream_iterator<int> os(std::cout, " ");
    std::cout << "GPU result : ";
    thrust::copy(y.begin(), y.end(), os);
    std::cout << std::endl;
   

    
    thrust::host_vector<my_variant> h_x = x;
    thrust::host_vector<int> h_y(3);
    //Visit the vector on the CPU
    thrust::transform(h_x.begin(), h_x.end(), h_y.begin(), extractor());
    std::cout << "CPU result : ";
    thrust::copy(h_y.begin(), h_y.end(), os);
    std::cout << std::endl;

    std::cout << "Truth      : 33 11 22" << std::endl;

    std::cout << "Held types: ";
    convertibles d = 10;
    reflector r;
    variant::apply_visitor(r, d); 
    d = 10.0f;
    variant::apply_visitor(r, d); 
    d = 10.0;
    variant::apply_visitor(r, d); 
    d = 10L;
    variant::apply_visitor(r, d); 
    std::cout << std::endl;

    std::cout << "True types: ";
    r(10);
    r(10.0f);
    r(10.0);
    r(10L);
    std::cout << std::endl;
    
}
