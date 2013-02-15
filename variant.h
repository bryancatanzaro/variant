#pragma once
#include <thrust/detail/type_traits.h>
#include "uninitialized.h"

namespace variant {
namespace detail {

template<bool c, typename T, typename E>
struct if_ {
    typedef E type;
};

template<typename T, typename E>
struct if_<true, T, E> {
    typedef T type;
};

struct nil{};

template<typename H, typename T=nil>
struct cons{};

template<typename T>
union storage;

template<typename Head, typename Tail>
union storage<cons<Head, Tail> > {
    Head head;
    storage<Tail> tail;
};

template<>
union storage<nil> {};

template<typename T0, typename T1=nil,
         typename T2=nil, typename T3=nil,
         typename T4=nil, typename T5=nil,
         typename T6=nil, typename T7=nil,
         typename T8=nil, typename T9=nil>
struct cons_type {
    typedef cons<T0, cons<T1, cons<T2, cons<T3, cons<T4,
            cons<T5, cons<T6, cons<T7, cons<T8, cons<T9, nil>
            > > > > > > > > > type;
};

//Wrapping works around the restrictions on datatypes that can
//be placed in a union, by wrapping objects in an
//uninitialized<T> type that is POD and therefore fits in a union.

template<typename T>
struct wrapped {
    typedef typename if_<
        thrust::detail::is_pod<T>::value ||
    thrust::detail::is_same<T, nil>::value,
        T, uninitialized<T> >::type type;
};

template<typename H, typename T>
struct wrapped<cons<H, T> > {
    typedef cons<
        typename wrapped<H>::type,
        typename wrapped<T>::type> type;
};

template<typename T>
struct is_wrapped {
    static const bool value = false;
};

template<typename T>
struct is_wrapped<uninitialized<T> > {
    static const bool value = true;
};

template<typename T>
struct unwrapped {
    typedef T type;
};

template<typename T>
struct unwrapped<uninitialized<T> > {
    typedef T type;
};

template<typename T, typename X, int R=0>
struct exact_match_index {
    static const bool exact_match =
        thrust::detail::is_same<T,
                                typename unwrapped<X>::type>::value;
    static const int value = exact_match ? R : -1;
};

template<typename T, typename Head, typename Tail, int R>
struct exact_match_index<T, cons<Head, Tail>, R> {
    static const int head_value = exact_match_index<T, Head, R>::value;    
    static const int value = head_value > 0 ? head_value :
        exact_match_index<T, Tail, R+1>::value;
};

template<typename T, typename X, int R=0>
struct first_convertible_index {
    static const bool convertible =
        thrust::detail::is_convertible<T,
                                       typename unwrapped<X>::type>::value;
    static const int value = convertible ? R : -1;
};

template<typename T, typename Head, typename Tail, int R>
struct first_convertible_index<T, cons<Head, Tail>, R> {
    static const int head_value = first_convertible_index<T, Head, R>::value;
    static const int value = head_value == R ? head_value :
        first_convertible_index<T, Tail, R+1>::value;
};

template<typename T, typename List,
         int exact_match=exact_match_index<T, List>::value,
         int convertible_match=first_convertible_index<T, List>::value> 
struct destination_index {
    static const int value = exact_match > 0 ? exact_match : convertible_match;
};

template<typename T, typename List, int E>
struct destination_index<T, List, E, -1> {
    //An error here indicates T is an incompatible type with the
    //variant
};

template<typename Cons, typename V, int R, bool Wrapped>
struct do_storage {
    __host__ __device__
    static void impl(storage<Cons>& storage, int& which, const V& value) {
        storage.head.construct(value);
        which = R;
    }
};

template<typename Cons, typename V, int R>
struct do_storage<Cons, V, R, false> {
#pragma hd_warning_disable
    __host__ __device__
    static void impl(storage<Cons>& storage, int& which, const V& value) {
        storage.head = value;
        which = R;
    }
};



template<typename V, typename List, int R=0, int D=destination_index<V, List>::value, bool Store=R==D>
struct initialize_storage{};

template<typename V, typename H, typename T, int R, int D>
struct initialize_storage<V, detail::cons<H, T>, R, D, true> {
    typedef detail::cons<H, T> Cons;

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which, const V& value) {
        do_storage<Cons, V, R, is_wrapped<H>::value>::impl(
            storage, which, value);
    }
};

template<typename V, typename H, typename T, int R, int D>
struct initialize_storage<V, detail::cons<H, T>, R, D, false> {
    typedef detail::cons<H, T> Cons;

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which, const V& value) {
        initialize_storage<V, T, R+1, D, R+1==D>::impl(storage.tail, which, value);
    }
};

template<typename List, int R=0>
struct destroy_storage{
    __host__ __device__
    static void impl(storage<List>& s, int which) {}
};

template<typename T>
__host__ __device__
void destroy(T&) {}


template<typename T>
__host__ __device__
void destroy(uninitialized<T>& wrapped) {
    wrapped.destroy();
}

template<typename Head, typename Tail, int R>
struct destroy_storage<cons<Head, Tail>, R> {
    __host__ __device__
    static void impl(storage<cons<Head, Tail> >& s, int which) {
        if (R==which) {
            destroy(s.head);
        } else {
            destroy_storage<Tail, R+1>::impl(s.tail, which);
        }
    }
        
};

}

template<typename T0,
         typename T1=detail::nil, typename T2=detail::nil,
         typename T3=detail::nil, typename T4=detail::nil,
         typename T5=detail::nil, typename T6=detail::nil,
         typename T7=detail::nil, typename T8=detail::nil,
         typename T9=detail::nil>
struct variant {
    typedef typename detail::cons_type<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>::type cons_type;
    typedef typename detail::wrapped<cons_type>::type wrapped_type;
    detail::storage<wrapped_type> m_storage;
    int m_which;

#pragma hd_warning_disable
    __host__ __device__
    variant() {
        detail::initialize_storage<T0, wrapped_type>::impl(
            m_storage, m_which, T0());
    }
    
    template<typename V>
    __host__ __device__
    variant(const V& value) {
        detail::initialize_storage<V, wrapped_type>::impl(
            m_storage, m_which, value);
    }

    __host__ __device__
    ~variant() {
        detail::destroy_storage<wrapped_type>::impl(m_storage, m_which);
    }

    template<typename V>
    __host__ __device__
    variant& operator=(const V& value) {
        detail::initialize_storage<V, wrapped_type>::impl(
            m_storage, m_which, value);
        return *this;
    }
    
    __host__ __device__
    int which() const {
        return m_which;
    }
};

template<typename Result=void>
struct static_visitor {
    typedef Result result_type;
};

namespace detail {

template<typename Fn, typename H, typename T>
__host__ __device__
typename Fn::result_type unwrap_apply(Fn fn,
                                      const detail::storage<
                                          detail::cons<H, T> >& storage) {
    return fn(storage.head);
}

template<typename Fn, typename H, typename T>
__host__ __device__
typename Fn::result_type unwrap_apply(Fn fn,
                                      const detail::storage<
                                          detail::cons<
                                              detail::uninitialized<H>,
                                              T> >& storage) {
    return fn(storage.head.get());
}




template<typename Fn, typename Cons, int R=0>
struct apply_to_variant{
#pragma hd_warning_disable
    template<typename S>
    __host__ __device__
    static typename Fn::result_type impl(Fn fn,
                                         const S& storage,
                                         const int& which) {
        if (R == which) {
            return unwrap_apply(fn, storage);
        } else {
            return typename Fn::result_type();
        }
    }

    __host__ __device__
    static typename Fn::result_type impl(Fn,
                                         const detail::storage<detail::nil>&,
                                         const int&) {
        return typename Fn::result_type();
    }
    
};

template<typename Fn, typename Head, typename Tail, int R>
struct apply_to_variant<Fn, cons<Head, Tail>, R> {

#pragma hd_warning_disable
    template<typename S>
    __host__ __device__
    static typename Fn::result_type impl(Fn fn,
                                         const S& storage,
                                         const int& which) {
        if (R == which) {
            return fn(storage.head);
        } else {
            return apply_to_variant<Fn, Tail, R+1>::impl(
                fn, storage.tail, which);
        }
    }
};

template<typename Fn, typename Tail, int R>
struct apply_to_variant<Fn, cons<nil, Tail>, R>{
    template<typename S>
    __host__ __device__
    static typename Fn::result_type impl(Fn fn,
                                         const S& storage,
                                         const int& which) {
        return typename Fn::result_type();
    }
};

}

template<typename Fn, typename Variant>
__host__ __device__
typename Fn::result_type apply_visitor(Fn fn, const Variant& v) {
    return detail::apply_to_variant<Fn,
                                    typename Variant::cons_type>::impl(fn, v.m_storage, v.m_which);
}

} //end namespace variant
