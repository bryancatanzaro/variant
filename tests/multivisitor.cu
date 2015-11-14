#include <cstdio>
#include <typeinfo>

#include <variant/variant.h>
#include <variant/multivisitors.h>

struct A{const static int val = 1;};
struct B{const static int val = 2;};

struct C{const static int val = 3;};
struct D{const static int val = 4;};

using AB = variant::variant<A, B>;
using CD = variant::variant<C, D>;

struct bivisitor {

    template<typename T0, typename T1>
    __host__ __device__
    void operator()(T0 t0, T1 t1) const {
        printf("%i, %i\n", t0.val, t1.val);
    }

};


int main() {

    AB ab = A{};
    CD cd = C{};
    variant::apply_visitor(bivisitor(), ab, cd);

    ab = B{};
    cd = C{};
    variant::apply_visitor(bivisitor(), ab, cd);

    ab = A{};
    cd = D{};
    variant::apply_visitor(bivisitor(), ab, cd);
    
    ab = B{};
    cd = D{};
    variant::apply_visitor(bivisitor(), ab, cd);
    

}
