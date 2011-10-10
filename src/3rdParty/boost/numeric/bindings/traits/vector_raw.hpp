//
//  Copyright (c) 2002,2003,2004
//  Toon Knapen, Kresimir Fresl, Joerg Walter, Karl Meerbergen
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_VECTOR_RAW_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_VECTOR_RAW_HPP

#include <cstddef> 
#include <boost/numeric/ublas/config.hpp> 
#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/vector.hpp> 
#endif 
#include <vector> 
#include <boost/numeric/bindings/traits/detail/array_impl.hpp> 

namespace boost { namespace numeric { namespace bindings { namespace traits {

  template <typename V>
  BOOST_UBLAS_INLINE
  int vector_size (const V &v) {
    return (int) v.size();
  }

  ////////////////////////////////////////////////////////////////
  // ublas::vector<> etc. 

  namespace ublas = boost::numeric::ublas; 

#if 0
  // MSVC seems to dislike overloads if there is `generic' template 
  template <typename V>
  BOOST_UBLAS_INLINE
  int vector_size (const ublas::vector_reference<V> &v) {
    return vector_size (v.expression());
  }
#endif 

#if 0
  // MSVC seems to dislike overloads if there is `generic' template 
  template <typename V>
  BOOST_UBLAS_INLINE
  int vector_stride (const V &v) { return 1; }
#endif 
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::vector<T,A> &v) { return 1; }
  template <typename V>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::vector_reference<V> &v) {
    return (int) vector_stride (v.expression());
  }
  template <typename T, std::size_t N>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::c_vector<T, N> &v) { return 1; }
  template <typename V>
  int vector_stride (const ublas::vector_slice<V>&);
  template <typename V>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::vector_range<V> &v) {
    return (int) vector_stride (v.data());
  }
  template <typename V>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::vector_slice<V> &v) {
    return (int) (v.stride() * vector_stride (v.data()));
  }


#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::vector<T,A>::value_type const* 
  vector_storage (const ublas::vector<T,A> &v) {
    return &v.data().begin()[0];
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::vector<T,A>::value_type const* 
  vector_storage_const (const ublas::vector<T,A> &v) {
    return &v.data().begin()[0];
  }
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::vector<T,A>::value_type* 
  vector_storage (ublas::vector<T,A> &v) {
    return &v.data().begin()[0];
  }

#if 0
  // MSVC seems to dislike overloads if there is `generic' template 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::const_pointer vector_storage (const V &v) {
    return &v.data().begin()[0];
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::const_pointer vector_storage_const (const V &v) {
    return &v.data().begin()[0];
  }
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::pointer vector_storage (V &v) {
    return &v.data().begin()[0];
  }
#endif 

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type const* 
  vector_storage (const ublas::vector_reference<V> &v) {
    return vector_storage (v.expression());
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type const* 
  vector_storage_const (const ublas::vector_reference<V> &v) {
    return vector_storage_const (v.expression());
  }
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type* vector_storage (ublas::vector_reference<V> &v) {
    return vector_storage (v.expression());
  }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename T, std::size_t N>
  BOOST_UBLAS_INLINE
  typename ublas::c_vector<T, N>::value_type const* 
  vector_storage (const ublas::c_vector<T, N> &v) {
    return v.data();
  }
#endif
#ifndef BOOST_MSVC
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename T, std::size_t N>
  BOOST_UBLAS_INLINE
  typename ublas::c_vector<T, N>::value_type const* 
  vector_storage_const (const ublas::c_vector<T, N> &v) {
    return v.data();
  }
  template <typename T, std::size_t N>
  BOOST_UBLAS_INLINE
  typename ublas::c_vector<T, N>::value_type* 
  vector_storage (ublas::c_vector<T, N> &v) {
    return v.data();
  }
#endif

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename V>
  typename V::value_type const* vector_storage (const ublas::vector_slice<V>&);
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type const* vector_storage (const ublas::vector_range<V> &v) {
    typename V::value_type const* ptr = vector_storage (v.data());
    ptr += v.start() * vector_stride (v.data());
    return ptr;
  }
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type const* vector_storage (const ublas::vector_slice<V> &v) {
    typename V::value_type const* ptr = vector_storage (v.data());
    ptr += v.start() * vector_stride (v.data());
    return ptr;
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename V>
  typename V::value_type const* 
  vector_storage_const (const ublas::vector_slice<V>&);
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type const* 
  vector_storage_const (const ublas::vector_range<V> &v) {
    typename V::value_type const* ptr = vector_storage_const (v.data());
    ptr += v.start() * vector_stride (v.data());
    return ptr;
  }
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type const* 
  vector_storage_const (const ublas::vector_slice<V> &v) {
    typename V::value_type const* ptr = vector_storage_const (v.data());
    ptr += v.start() * vector_stride (v.data());
    return ptr;
  }
  template <typename V>
  typename V::value_type* vector_storage (ublas::vector_slice<V>&);
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type* vector_storage (ublas::vector_range<V> &v) {
    typename V::value_type* ptr = vector_storage (v.data());
    ptr += v.start() * vector_stride (v.data());
    return ptr;
  }
  template <typename V>
  BOOST_UBLAS_INLINE
  typename V::value_type* vector_storage (ublas::vector_slice<V> &v) {
    typename V::value_type* ptr = vector_storage (v.data());
    ptr += v.start() * vector_stride (v.data());
    return ptr;
  }


  //////////////////////////////////////////////////////////////////
  // std::vector<> 

  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  int vector_stride (const std::vector<T,A> &v) { return 1; }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  typename std::vector<T, A>::value_type const* 
  vector_storage (const std::vector<T, A> &v) { return &v.front(); }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  typename std::vector<T, A>::value_type const* 
  vector_storage_const (const std::vector<T, A> &v) { return &v.front(); }
  template <typename T, typename A>
  BOOST_UBLAS_INLINE
  typename std::vector<T, A>::value_type* vector_storage (std::vector<T, A> &v) {
    return &v.front();
  }


  //////////////////////////////////////////////////////////////////
  // bindings::traits::detail::array<>

  template <typename T>
  BOOST_UBLAS_INLINE
  int vector_stride (const detail::array<T> &a) { return 1; }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename T> 
  BOOST_UBLAS_INLINE
  const T* vector_storage (const detail::array<T> &a) { return a.storage(); }
#endif 
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename T> 
  BOOST_UBLAS_INLINE
  const T* vector_storage_const (const detail::array<T> &a) { 
    return a.storage(); 
  }
  template <typename T> 
  BOOST_UBLAS_INLINE
  T* vector_storage (detail::array<T> &a) { return a.storage(); }


}}}}

#endif
