Variant
===

Boost provides
[`boost::variant`](http://www.boost.org/doc/libs/1_53_0/doc/html/variant.html)
to provide a type-checked discriminated union type.

This library provides a simple `variant` type similar in concept to
`boost::variant`, that works in CUDA, for use in GPU code as well as
CPU code.

This type is not as fully featured as `boost::variant`: it does not
support recursive variants, nor does it support variants of references.
However, it does support variants of non-POD data types, unlike C++03 unions.

`variant`s are accessed by means of functor classes that
overload `operator()` for all types in the variant, and then are
applied using `apply_visitor()`, as shown in the example.  Compile-time
errors result from trying to visit `variant`s with functors
that do not provide overloads for all types in the `variant`. This is
an important safety benefit.

To extract a known type `T` from a variant, use `variant::get<T>`.
On the CPU, this will throw a `std::runtime_exception` if the variant
currently contains a type other than `T`. 

On the GPU, if `get<T>` fails at runtime, the program will be terminated -
it is treated as a catastrophic error.

This code has been tested on Linux, which NVCC 7.0 and g++ 4.8.

Usage
===
```c++
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

//Declare a variant type
typedef variant::variant<int, foo> my_variant;

//Accessing a variant type is done through a visitor functor.
//This provides type safety: at compile time you will get an
//error if you try to access a variant type by a functor.
//that does not know how to operate on all types which the
//variant may contain.
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
//This is just a convenience when using Thrust
struct extractor {
    typedef int result_type;
    __host__ __device__
    int operator()(const my_variant& v) const {
        //The actual access to the variant is done by
        //calling apply_visitor, with two arguments:
        //1. The static_visitor which operates on the variant
        //2. The variant being accessed
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
   
    
    thrust::host_vector<my_variant> h_x = x;
    thrust::host_vector<int> h_y(2);

    //Visit the vector on the CPU
    thrust::transform(h_x.begin(), h_x.end(), h_y.begin(), extractor());
    std::cout << "CPU result : ";
    thrust::copy(h_y.begin(), h_y.end(), os);
    std::cout << std::endl;

    std::cout << "Truth      : 11 22" << std::endl;
    
}
```