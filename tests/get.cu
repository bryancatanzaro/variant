#include <variant/variant.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <stdexcept>

//Non-pod type
struct foo{
    int o;
    __host__ __device__
    foo(int _o) : o(_o) {};
};

typedef variant::variant<int, foo> my_variant;

template<typename R>
struct extractor {
    typedef R result_type;

    template<typename V>
    __host__ __device__
    R operator()(const V& v) const {
        return variant::get<R>(v);
    }
};

int main() {
    thrust::host_vector<my_variant> x(1);
    x[0] = 1;
    int r = variant::get<int>(x[0]);
    std::cout << r << std::endl;
    try {
        foo f = variant::get<foo>(x[0]);
    } catch(std::runtime_error e) {}


    x[0] = foo(2);
    foo f = variant::get<foo>(x[0]);
    std::cout << f.o << std::endl;

    try {
        r = variant::get<int>(x[0]);
    } catch(std::runtime_error e) {}


    

    thrust::device_vector<my_variant> d(1);
    d[0] = 10;
    thrust::device_vector<int> y(1);
    thrust::transform(d.begin(), d.end(), y.begin(), extractor<int>());

    r = y[0];
    std::cout << r << std::endl;

    //The following code will cause a catastrophic assertion at runtime
    //Because it leads to a bad get.
    // thrust::device_vector<foo> z(1, foo(20));
    // thrust::transform(d.begin(), d.end(), z.begin(), extractor<foo>());
    
}

