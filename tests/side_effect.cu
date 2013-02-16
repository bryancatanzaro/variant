#include <variant/variant.h>
#include <cstdio>
#include <thrust/device_vector.h>
struct side_effect {
    __host__ __device__
    side_effect() {
        printf("+");
    }
    __host__ __device__
    side_effect(const side_effect&) {
        printf("-");
    }
    __host__ __device__
    ~side_effect() {
        printf(".");
    }
};

typedef variant::variant<side_effect, int> my_variant;

int main() {
    printf("There should be as many . characters as + & - characters\n\n");
    my_variant m;
    m = 10;
    m = side_effect();

    thrust::device_vector<my_variant> v(1);
    v[0] = 5;
    v[0] = side_effect();
    printf("\n");
}
