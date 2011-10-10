//
//  Copyright (c) 2002-2003
//  Toon Knapen, Kresimir Fresl, Joerg Walter
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_SYMM_HERM_RAW_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_SYMM_HERM_RAW_HPP

#include <boost/numeric/bindings/traits/matrix_raw.hpp> 
#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/symmetric.hpp> 
#  include <boost/numeric/ublas/hermitian.hpp> 
#endif 

namespace boost { namespace numeric { namespace bindings { namespace traits {

  namespace ublas = boost::numeric::ublas; 

  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::symmetric_adaptor<M, F> &m) {
    return bindings::traits::leading_dimension (m.data());
  }

  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  int leading_dimension (const ublas::hermitian_adaptor<M, F> &m) {
    return bindings::traits::leading_dimension (m.data());
  }



  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  int matrix_storage_size (const ublas::symmetric_adaptor<M, F> &m) {
    return matrix_storage_size (m.data()); 
  }

  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  int matrix_storage_size (const ublas::hermitian_adaptor<M, F> &m) {
    return matrix_storage_size (m.data()); 
  }

  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  int matrix_storage_size (const ublas::symmetric_matrix<T,F1,F2,A> &m) {
    return (int) ((m.size1() * (m.size1() + 1)) / 2); 
  }

  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  int matrix_storage_size (const ublas::hermitian_matrix<T,F1,F2,A> &m) {
    return (int) ((m.size1() * (m.size1() + 1)) / 2); 
  }



#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::symmetric_matrix<T,F1,F2,A>::const_pointer 
  matrix_storage (const ublas::symmetric_matrix<T,F1,F2,A> &m) {
    return &m.data().begin()[0];
  }
#endif
  // We need data_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::symmetric_matrix<T,F1,F2,A>::const_pointer 
  matrix_storage_const (const ublas::symmetric_matrix<T,F1,F2,A> &m) {
    return &m.data().begin()[0];
  }
  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::symmetric_matrix<T,F1,F2,A>::pointer 
  matrix_storage (ublas::symmetric_matrix<T,F1,F2,A> &m) {
    return &m.data().begin()[0];
  }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage (const ublas::symmetric_adaptor<M, F> &m) {
    return matrix_storage (m.data()); 
  }
#endif
  // We need data_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage_const (const ublas::symmetric_adaptor<M, F> &m) {
    return matrix_storage_const (m.data()); 
  }
  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  typename M::pointer matrix_storage (ublas::symmetric_adaptor<M, F> &m) {
    return matrix_storage (m.data()); 
  }


#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::hermitian_matrix<T,F1,F2,A>::const_pointer 
  matrix_storage (const ublas::hermitian_matrix<T,F1,F2,A> &m) {
    return &m.data().begin()[0];
  }
#endif
  // We need data_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::hermitian_matrix<T,F1,F2,A>::const_pointer 
  matrix_storage_const (const ublas::hermitian_matrix<T,F1,F2,A> &m) {
    return &m.data().begin()[0];
  }
  template<typename T, typename F1, typename F2, typename A>
  BOOST_UBLAS_INLINE
  typename ublas::hermitian_matrix<T,F1,F2,A>::pointer 
  matrix_storage (ublas::hermitian_matrix<T,F1,F2,A> &m) {
    return &m.data().begin()[0];
  }

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage (const ublas::hermitian_adaptor<M, F> &m) {
    return matrix_storage (m.data()); 
  }
#endif
  // We need data_const() mostly due to MSVC 6.0.
  // But how shall we write portable code otherwise?
  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  typename M::const_pointer 
  matrix_storage_const (const ublas::hermitian_adaptor<M, F> &m) {
    return matrix_storage_const (m.data()); 
  }
  template <typename M, typename F>
  BOOST_UBLAS_INLINE
  typename M::pointer matrix_storage (ublas::hermitian_adaptor<M, F> &m) {
    return matrix_storage (m.data()); 
  }

  namespace detail {

    inline char m_uplo_tag (ublas::upper_tag const&) { return 'U'; } 
    inline char m_uplo_tag (ublas::lower_tag const&) { return 'L'; } 

  }

  template <typename SymmM> 
  inline 
  char matrix_uplo_tag (SymmM&) {
      typedef typename SymmM::packed_category uplo_t; 
      return detail::m_uplo_tag (uplo_t());
  }
  

}}}}

#endif
