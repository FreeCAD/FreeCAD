//
//  Copyright (c) 2002-2003
//  Toon Knapen, Kresimir Fresl, Joerg Walter
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_MATRIX_RAW_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_MATRIX_RAW_HPP

#include <cstddef> 
#include <boost/numeric/ublas/config.hpp> 
#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/matrix.hpp> 
#endif 

namespace boost { namespace numeric { namespace bindings { namespace traits {

  namespace ublas = boost::numeric::ublas; 

  // size: 

  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_size1 (const M &m) {
    return (int) m.size1();
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_size2 (const M &m) {
    return (int) m.size2();
  }

#if 0
  // MSVC seems to dislike overloads if there is 'generic' template 
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_size1 (const ublas::matrix_reference<M> &m) {
    return matrix_size1 (m.expression());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_size2 (const ublas::matrix_reference<M> &m) {
    return matrix_size2 (m.expression());
  }
#endif // 0


  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  int matrix_storage_size (const ublas::matrix<T,F,A>& m) {
    return (int) (m.size1() * m.size2());
  } 
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  int matrix_storage_size (const ublas::c_matrix<T, M, N> &m) {
    return (int) (M * N);
  }


  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::matrix<T,F,A> &m, ublas::row_major_tag) {
    return (int) m.size2();
  }
  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::matrix<T,F,A> &m, ublas::column_major_tag) {
    return (int) m.size1();
  }
  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::matrix<T,F,A> &m) {
    typedef ublas::matrix<T,F,A> matrix_t; 
    return bindings::traits::leading_dimension
      (m, BOOST_UBLAS_TYPENAME matrix_t::orientation_category());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::matrix_reference<M> &m) {
    return bindings::traits::leading_dimension (m.expression());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::matrix_range<M> &m) {
    return bindings::traits::leading_dimension (m.data());
  }
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::c_matrix<T, M, N> &m) {
    return (int) N;
  }


  // stride: 

#if 0
  // MSVC seems to dislike overloads if there is 'generic' template 
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride1 (const M &m) {
    typedef typename M::functor_type functor_type;
    return (int) functor_type::one1 (m.size1(), m.size2());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride2 (const M &m) {
    typedef typename M::functor_type functor_type;
    return (int) functor_type::one2 (m.size1(), m.size2());
  }
#endif // 0

  template <typename M, typename F, typename A>
  BOOST_UBLAS_INLINE
  int matrix_stride1 (const ublas::matrix<M,F,A> &m) {
    return (int) F::one1 (m.size1(), m.size2());
  }
  template <typename M, typename F, typename A>
  BOOST_UBLAS_INLINE
  int matrix_stride2 (const ublas::matrix<M,F,A> &m) {
    return (int) F::one2 (m.size1(), m.size2());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride1 (const ublas::matrix_reference<M> &m) {
    return matrix_stride1 (m.expression());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride2 (const ublas::matrix_reference<M> &m) {
    return matrix_stride2 (m.expression());
  }
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  int matrix_stride1 (const ublas::c_matrix<T, M, N> &m) { return (int) N; }
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  int matrix_stride2 (const ublas::c_matrix<T, M, N> &m) { return 1; }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride1 (const ublas::matrix_range<M> &m) {
    return matrix_stride1 (m.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride2 (const ublas::matrix_range<M> &m) {
    return matrix_stride2 (m.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride1 (const ublas::matrix_slice<M> &m) {
    return (int) (m.stride1() * matrix_stride1 (m.data()));
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int matrix_stride2 (const ublas::matrix_slice<M> &m) {
    return (int) (m.stride2() * matrix_stride2 (m.data()));
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::matrix_row<M> &v) {
    return matrix_stride2 (v.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  int vector_stride (const ublas::matrix_column<M> &v) {
    return matrix_stride1 (v.data());
  }


  // storage: 

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::matrix<T,F,A>::const_pointer 
  matrix_storage (const ublas::matrix<T,F,A> &m) {
    return &m.data().begin()[0];
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::matrix<T,F,A>::const_pointer 
  matrix_storage_const (const ublas::matrix<T,F,A> &m) {
    return &m.data().begin()[0];
  }
  template <typename T, typename F, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::matrix<T,F,A>::pointer 
  matrix_storage (ublas::matrix<T,F,A> &m) {
    return &m.data().begin()[0];
  }

#if 0
  // MSVC seems to dislike overloads if there is 'generic' template 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer matrix_storage (const M &m) {
    return &m.data().begin()[0];
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer matrix_storage_const (const M &m) {
    return &m.data().begin()[0];
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::pointer matrix_storage (M &m) {
    return &m.data().begin()[0];
  }
#endif // 0 

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage (const ublas::matrix_reference<M> &m) {
    return matrix_storage (m.expression ());
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage_const (const ublas::matrix_reference<M> &m) {
    return matrix_storage_const (m.expression ());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::pointer matrix_storage (ublas::matrix_reference<M> &m) {
    return matrix_storage (m.expression ());
  }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  typename ublas::c_matrix<T, M, N>::const_pointer 
  matrix_storage (const ublas::c_matrix<T, M, N> &m) {
    return m.data();
  }
#endif
#ifndef BOOST_MSVC
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  typename ublas::c_matrix<T, M, N>::const_pointer 
  matrix_storage_const (const ublas::c_matrix<T, M, N> &m) {
    return m.data();
  }
  template <typename T, std::size_t M, std::size_t N>
  BOOST_UBLAS_INLINE
  typename ublas::c_matrix<T, M, N>::pointer 
  matrix_storage (ublas::c_matrix<T, M, N> &m) {
    return m.data();
  }
#endif

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage (const ublas::matrix_range<M> &m) {
    return matrix_storage (m.data()) 
      + m.start1() * matrix_stride1 (m.data()) 
      + m.start2() * matrix_stride2 (m.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage (const ublas::matrix_slice<M> &m) {
    return matrix_storage (m.data()) 
      + m.start1() * matrix_stride1 (m.data ()) 
      + m.start2() * matrix_stride2 (m.data ());
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage_const (const ublas::matrix_range<M> &m) {
    return matrix_storage_const (m.data()) 
      + m.start1() * matrix_stride1 (m.data ()) 
      + m.start2() * matrix_stride2 (m.data ());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage_const (const ublas::matrix_slice<M> &m) {
    return matrix_storage_const (m.data()) 
      + m.start1() * matrix_stride1 (m.data ()) 
      + m.start2() * matrix_stride2 (m.data ());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::pointer matrix_storage (ublas::matrix_range<M> &m) {
    return matrix_storage (m.data()) 
      + m.start1() * matrix_stride1 (m.data ()) 
      + m.start2() * matrix_stride2 (m.data ());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::pointer matrix_storage (ublas::matrix_slice<M> &m) {
    return matrix_storage (m.data()) 
      + m.start1() * matrix_stride1 (m.data ()) 
      + m.start2() * matrix_stride2 (m.data ());
  }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  vector_storage (const ublas::matrix_row<M> &v) {
    return matrix_storage (v.data()) 
      + v.index() * matrix_stride1 (v.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  vector_storage (const ublas::matrix_column<M> &v) {
    return matrix_storage (v.data()) 
      + v.index() * matrix_stride2 (v.data());
  }
#endif
  // We need storage_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  vector_storage_const (const ublas::matrix_row<M> &v) {
    return matrix_storage_const (v.data()) 
      + v.index() * matrix_stride1 (v.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  vector_storage_const (const ublas::matrix_column<M> &v) {
    return matrix_storage_const (v.data()) 
      + v.index() * matrix_stride2 (v.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::pointer vector_storage (ublas::matrix_row<M> &v) {
    return matrix_storage (v.data()) 
      + v.index() * matrix_stride1 (v.data());
  }
  template <typename M>
  BOOST_UBLAS_INLINE
  typename M::pointer vector_storage (ublas::matrix_column<M> &v) {
    return matrix_storage (v.data()) 
      + v.index() * matrix_stride2 (v.data());
  }

}}}}

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_NO_SYMMETRIC_TRAITS 

#include <boost/numeric/bindings/traits/symm_herm_raw.hpp> 

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_NO_SYMMETRIC_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_MATRIX_RAW_HPP
