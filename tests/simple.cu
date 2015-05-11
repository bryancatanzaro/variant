#include <variant/variant.h>
#include <iostream>
#include <thrust/copy.h>
#include <thrust/device_vector.h>

//Non-pod type
struct foo{
    int o;
    __host__ __device__
    foo(int _o) : o(_o) {};
};

typedef variant::variant<int, foo> my_variant;

struct my_visitor {
    __host__ __device__
    int operator()(const int& i) const {
        return i + 10;
    }
    __host__ __device__
    int operator()(const foo& i) const {
        return i.o + 20;
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




int main() {
    
    //Make a vector of variants on the GPU
    thrust::device_vector<my_variant> x(2);

    //Initialize their elements
    x[0] = 1;
    x[1] = foo(2);
    thrust::device_vector<int> y(2);
    //Visit the vector on the GPU
    thrust::transform(x.begin(), x.end(), y.begin(), extractor());
    std::ostream_iterator<int> os(std::cout, " ");
    std::cout << "GPU result : ";
    thrust::copy(y.begin(), y.end(), os);
    std::cout << std::endl;
   

    
    // thrust::host_vector<my_variant> h_x(2);
    // h_x[0] = 1;
    // h_x[1] = foo(2);
    thrust::host_vector<my_variant> h_x = x;
    thrust::host_vector<int> h_y(2);
    //Visit the vector on the CPU
    thrust::transform(h_x.begin(), h_x.end(), h_y.begin(), extractor());
    std::cout << "CPU result : ";
    thrust::copy(h_y.begin(), h_y.end(), os);
    std::cout << std::endl;

    std::cout << "Truth      : 11 22" << std::endl;


    
}
