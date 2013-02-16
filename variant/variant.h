#pragma once
#include <thrust/detail/type_traits.h>
#include <variant/detail/uninitialized.h>

namespace variant {

//This is here by analogy to boost::static_visitor.
//It could also be a std::unary_function or similar.
template<typename Result=void>
struct static_visitor {
    typedef Result result_type;
};

namespace detail {

//Thrust doesn't provide if_
template<bool c, typename T, typename E>
struct if_ {
    typedef E type;
};

template<typename T, typename E>
struct if_<true, T, E> {
    typedef T type;
};

//A type list
struct nil{};

template<typename H, typename T=nil>
struct cons{
    typedef H head_type;
    typedef T tail_type;
};

//We will store objects in a union.
//non-pod data types will be wrapped
//due to restrictions on what types can be
//placed in unions.
template<typename T>
union storage;

template<typename Head, typename Tail>
union storage<cons<Head, Tail> > {
    Head head;
    storage<Tail> tail;
};

template<>
union storage<nil> {};


//To disambiguate constructors, we need a unique type for each
//potential element of the variant, except the first, which is
//guaranteed to exist.
struct nil1 : public nil{};
struct nil2 : public nil{};
struct nil3 : public nil{};
struct nil4 : public nil{};
struct nil5 : public nil{};
struct nil6 : public nil{};
struct nil7 : public nil{};
struct nil8 : public nil{};
struct nil9 : public nil{};

//Create a type list from a set of types
template<typename T0, typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6, typename T7, typename T8, typename T9>
struct cons_type {
    typedef cons<T0, cons<T1, cons<T2, cons<T3, cons<T4,
            cons<T5, cons<T6, cons<T7, cons<T8, cons<T9, nil>
            > > > > > > > > > type;
};

//Derive the length of a type list
template<typename Cons>
struct cons_length {
    static const int value = thrust::detail::is_convertible<
        typename Cons::head_type, nil>::value ? 0 :
    1 + cons_length<typename Cons::tail_type>::value;
};

template<>
struct cons_length<nil> {
    static const int value = 0;
};

//Store a flat set of types.
//We use this only for construction: it allows us to use standard
//overload resolution for conversions, to choose the best type a
//variant will hold, given some type to store in it.
template<typename S0, typename S1, typename S2, typename S3, typename S4,
         typename S5, typename S6, typename S7, typename S8, typename S9>
struct flat_type {
    typedef S0 T0;
    typedef S1 T1;
    typedef S2 T2;
    typedef S3 T3;
    typedef S4 T4;
    typedef S5 T5;
    typedef S6 T6;
    typedef S7 T7;
    typedef S8 T8;
    typedef S9 T9;
};

//These overloads call a function on an object held in storage
//The object is unwrapped if it was wrapped due to being non-pod
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



//Call a function on a variant
template<typename Fn, typename Cons, int R=0, int L=cons_length<Cons>::value,
         bool stop = R==L-1>
struct apply_to_variant{};

template<typename Fn, typename Head, typename Tail, int R, int L>
struct apply_to_variant<Fn, cons<Head, Tail>, R, L, false> {

#pragma hd_warning_disable
    template<typename S>
    __host__ __device__
    static typename Fn::result_type impl(Fn fn,
                                         const S& storage,
                                         const int& which) {
        if (R == which) {
            return unwrap_apply(fn, storage);
        } else {
            return apply_to_variant<Fn, Tail, R+1, L>::impl(
                fn, storage.tail, which);
        }
    }
};

template<typename Fn, typename Head, typename Tail, int R, int L>
struct apply_to_variant<Fn, cons<Head, Tail>, R, L, true>{
    template<typename S>
    __host__ __device__
    static typename Fn::result_type impl(Fn fn,
                                         const S& storage,
                                         const int& which) {
        return unwrap_apply(fn, storage);
    }
};

} //end namespace detail

//Apply visitor to variant
template<typename Fn, typename Variant>
__host__ __device__
typename Fn::result_type apply_visitor(Fn fn, const Variant& v) {
    return detail::apply_to_variant<Fn,
                                    typename Variant::wrapped_type>::
        impl(fn, v.m_storage, v.m_which);
}


namespace detail {

//Wrapping works around the restrictions on datatypes that can
//be placed in a union, by wrapping objects in an
//uninitialized<T> type that is POD and therefore fits in a union.

template<typename T>
struct wrapped {
    typedef typename if_<
        thrust::detail::is_pod<T>::value ||
        thrust::detail::is_convertible<T, nil>::value,
        T, uninitialized<T> >::type type;
};

template<typename H, typename T>
struct wrapped<cons<H, T> > {
    typedef cons<
        typename wrapped<H>::type,
        typename wrapped<T>::type> type;
};

//Is a type wrapped?
template<typename T>
struct is_wrapped {
    static const bool value = false;
};

template<typename T>
struct is_wrapped<uninitialized<T> > {
    static const bool value = true;
};

//Unwrap a type, if it was wrapped.
template<typename T>
struct unwrapped {
    typedef T type;
};

template<typename T>
struct unwrapped<uninitialized<T> > {
    typedef T type;
};

//Construct an object in storage
//Optionally by invoking construct() on the wrapper
//Or by assignment for POD
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

//This loop indexes into the storage class to find the right
//place to put the data.
template<typename Cons, typename V, int D, int R=0, bool store=D==R>
struct iterate_do_storage {
    __host__ __device__
    static void impl(storage<Cons>& storage, int& which, const V& value) {
        do_storage<Cons, V, R, is_wrapped<typename Cons::head_type>::value>::
            impl(storage, which, value);
    }
};

template<typename Cons, typename V, int D, int R>
struct iterate_do_storage<Cons, V, D, R, false> {
    __host__ __device__
    static void impl(storage<Cons>& storage, int& which, const V& value) {
        iterate_do_storage<typename Cons::tail_type, V, D, R+1>::
            impl(storage.tail, which, value);
    }
};


//Given a V value, the flat type list of variant, and the
//wrapped, recursive form type list, initialize the storage
template<typename V,
         typename Flat,
         typename Cons>
struct initialize_storage{
    //All these overloads expose the complete set of types in the
    //variant
    //Accordingly, during assignment or initialization, the type stored
    //in the variant will be chosen by standard overload resolution
    //rules.
    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T0& value) {
        iterate_do_storage<Cons, V, 0>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T1& value) {
        iterate_do_storage<Cons, V, 1>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T2& value) {
        iterate_do_storage<Cons, V, 2>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T3& value) {
        iterate_do_storage<Cons, V, 3>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T4& value) {
        iterate_do_storage<Cons, V, 4>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T5& value) {
        iterate_do_storage<Cons, V, 5>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T6& value) {
        iterate_do_storage<Cons, V, 6>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T7& value) {
        iterate_do_storage<Cons, V, 7>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T8& value) {
        iterate_do_storage<Cons, V, 8>::impl(storage, which, value);
    }

    __host__ __device__
    static void impl(storage<Cons>& storage, int& which,
                     const typename Flat::T9& value) {
        iterate_do_storage<Cons, V, 9>::impl(storage, which, value);
    }

};

//These overloads call the destructor, if necessary
//POD has no constructor, so destroy does nothing
template<typename T>
__host__ __device__
void destroy(T&) {}

//Wrapped data has a destructor, call it through uninitialized
template<typename T>
__host__ __device__
void destroy(uninitialized<T>& wrapped) {
    wrapped.destroy();
}

//Iterate through storage, call the correct destructor
template<typename List, int R=0>
struct destroy_storage{
    __host__ __device__
    static void impl(storage<List>& s, int which) {}
};

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

//Copy construction from another variant requires us to
//visit the other variant.  Each type in the other variant
//must be convertible to one of the types held in the destination
//variant.  This visitor exposes all the types held in the other
//variant, and calls initialize_storage for each one.

//Errors here indicate that the variant being copied from
//contains at least one type which is not convertible to
//any types in the variant being copied to.
template<typename Flat, typename Cons>
struct copy_construct_variant : public static_visitor<void> {
    typedef storage<Cons> storage_type;
    storage_type& m_storage;
    int& m_which;
    __host__ __device__ copy_construct_variant(storage_type& storage,
                                               int& which) :
        m_storage(storage), m_which(which) {}
    
    template<typename V>
    __host__ __device__
    void operator()(const V& value) const {
        initialize_storage<V, Flat, Cons>::impl(m_storage, m_which, value);
    }
};


}

//The variant itself
template<typename T0,
         typename T1=detail::nil1, typename T2=detail::nil2,
         typename T3=detail::nil3, typename T4=detail::nil4,
         typename T5=detail::nil5, typename T6=detail::nil6,
         typename T7=detail::nil7, typename T8=detail::nil8,
         typename T9=detail::nil9>
struct variant {
    typedef typename
    detail::cons_type<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>::type cons_type;
    typedef detail::flat_type<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> flat_type;
    
    typedef typename detail::wrapped<cons_type>::type wrapped_type;
    typedef detail::storage<wrapped_type> storage_type;
    storage_type m_storage;
    int m_which;

#pragma hd_warning_disable
    __host__ __device__
    variant() {
        detail::initialize_storage<T0, flat_type, wrapped_type>::impl(
            m_storage, m_which, T0());
    }
    
    template<typename V>
    __host__ __device__
    variant(const V& value) {
        detail::initialize_storage<V, flat_type, wrapped_type>::impl(
            m_storage, m_which, value);
    }

    __host__ __device__
    variant(const variant& value) {
        apply_visitor(detail::copy_construct_variant<flat_type, wrapped_type>
                      (m_storage, m_which), value);
    }
    
    
    template<typename S0, typename S1, typename S2, typename S3, typename S4,
             typename S5, typename S6, typename S7, typename S8, typename S9>
    __host__ __device__
    variant(const variant<S0, S1, S2, S3, S4, S5, S6, S7, S8, S9>& value) {
        apply_visitor(detail::copy_construct_variant<flat_type, wrapped_type>
                      (m_storage, m_which), value);
    }

    __host__ __device__
    ~variant() {
        detail::destroy_storage<wrapped_type>::impl(m_storage, m_which);
    }

    template<typename V>
    __host__ __device__
    variant& operator=(const V& value) {
        detail::destroy_storage<wrapped_type>::impl(m_storage, m_which);
        detail::initialize_storage<V, flat_type, wrapped_type>::impl(
            m_storage, m_which, value);
        return *this;
    }
    
    __host__ __device__
    int which() const {
        return m_which;
    }
};


} //end namespace variant
