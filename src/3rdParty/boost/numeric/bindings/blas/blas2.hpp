//
//  Copyright Toon Knapen and Kresimir Fresl 2003
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_BINDINGS_BLAS_BLAS2_HPP
#define BOOST_BINDINGS_BLAS_BLAS2_HPP

#include <boost/numeric/bindings/blas/blas2_overloads.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/transpose.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <cassert> 

namespace boost { namespace numeric { namespace bindings { namespace blas {

  // y <- alpha * op (A) * x + beta * y
  // op (A) == A || A^T || A^H
  // ! CAUTION this function assumes that all matrices involved are column-major matrices
  template < typename matrix_type, typename vector_type_x, typename vector_type_y, typename value_type >
  inline
  void gemv(const char TRANS, 
	    const value_type& alpha, 
	    const matrix_type &a, 
	    const vector_type_x &x, 
	    const value_type& beta,
	    vector_type_y &y
	    )
  {
    // precondition: matrix_type must be dense or dense_proxy
    /* not all compilers can handle the traits
    BOOST_STATIC_ASSERT( ( boost::is_same< typename mtraits::matrix_structure,
                                           boost::numeric::bindings::traits::general_t
                           >::value ) ) ;
    */

    const int m = traits::matrix_size1( a ) ;
    const int n = traits::matrix_size2( a ) ;
    assert ( traits::vector_size( x ) >= (TRANS == traits::NO_TRANSPOSE ? n : m) ) ; 
    assert ( traits::vector_size( y ) >= (TRANS == traits::NO_TRANSPOSE ? m : n) ) ; 
    const int lda = traits::leading_dimension( a ) ; 
    const int stride_x = traits::vector_stride( x ) ;
    const int stride_y = traits::vector_stride( y ) ;

    const value_type *a_ptr = traits::matrix_storage( a ) ;
    const value_type *x_ptr = traits::vector_storage( x ) ;
    value_type *y_ptr = traits::vector_storage( y ) ;

    detail::gemv( TRANS, m, n, alpha, a_ptr, lda, x_ptr, stride_x, beta, y_ptr, stride_y );
  }

  // A <- alpha * x * trans(y) ( outer product ), alpha, x and y are real-valued 
  // ! CAUTION this function assumes that all matrices involved are column-major matrices
  template < typename vector_type_x, typename vector_type_y, typename value_type, typename matrix_type >
  inline
  void ger( const value_type& alpha, 
            const vector_type_x &x, 
            const vector_type_y &y,
            matrix_type &a 
            )
  {
    // precondition: matrix_type must be dense or dense_proxy
    /* not all compilers can handle the traits
    BOOST_STATIC_ASSERT( ( boost::is_same< typename mtraits::matrix_structure,
                                           boost::numeric::bindings::traits::general_t
                           >::value ) ) ;
    */

    const int m = traits::matrix_size1( a ) ;
    const int n = traits::matrix_size2( a ) ;
    assert ( traits::vector_size( x ) <= m ) ; 
    assert ( traits::vector_size( y ) <= n ) ; 
    const int lda = traits::leading_dimension( a ) ; 
    const int stride_x = traits::vector_stride( x ) ;
    const int stride_y = traits::vector_stride( y ) ;

    const value_type *x_ptr = traits::vector_storage( x ) ;
    const value_type *y_ptr = traits::vector_storage( y ) ;
    value_type *a_ptr = traits::matrix_storage( a ) ;
    
    detail::ger( m, n, alpha, x_ptr, stride_x, y_ptr, stride_y, a_ptr, lda );
  }
/*
  // A <- alpha * x * trans(y) ( outer product ), alpha, x and y are complex-valued 
  template < typename vector_type_x, typename vector_type_y, typename value_type, typename matrix_type >
  inline
  void geru( const value_type& alpha, 
             const vector_type_x &x, 
             const vector_type_y &y,
             matrix_type &a 
             )
  {
    // precondition: matrix_type must be dense or dense_proxy
//    not all compilers can handle the traits
//    BOOST_STATIC_ASSERT( ( boost::is_same< typename mtraits::matrix_structure,
//                                           boost::numeric::bindings::traits::general_t
//                           >::value ) ) ;
    

//    BOOST_STATIC_ASSERT( ( boost::is_same< x.value_type(), FEMTown::Complex() >::value ) ) ;
    const int m = traits::matrix_size1( a ) ;
    const int n = traits::matrix_size2( a ) ;
    assert ( traits::vector_size( x ) <= m ) ; 
    assert ( traits::vector_size( y ) <= n ) ; 
    const int lda = traits::leading_dimension( a ) ; 
    const int stride_x = traits::vector_stride( x ) ;
    const int stride_y = traits::vector_stride( y ) ;

    const value_type *x_ptr = traits::vector_storage( x ) ;
    const value_type *y_ptr = traits::vector_storage( y ) ;
    value_type *a_ptr = traits::matrix_storage( a ) ;
    
    detail::geru( m, n, alpha, x_ptr, stride_x, y_ptr, stride_y, a_ptr, lda );
  }
*/
  /*
  // y <- alpha * A * x + beta * y 
  template < typename matrix_type, typename vector_type_x, typename vector_type_y >
  inline 
  void gemv(const typename traits::matrix_traits<matrix_type>::value_type &alpha, 
	    const matrix_type &a, 
	    const vector_type_x &x, 
	    const typename traits::vector_traits<vector_type_y>::value_type &beta,
	    vector_type_y &y
	    )
  {
    gemv( traits::NO_TRANSPOSE, alpha, a, x, beta, y ); 
  }


  // y <- A * x
  template < typename matrix_type, typename vector_type_x, typename vector_type_y >
  inline 
  void gemv(const matrix_type &a, const vector_type_x &x, vector_type_y &y)
  {
    typedef typename traits::matrix_traits<matrix_type>::value_type val_t; 
    gemv( traits::NO_TRANSPOSE, (val_t) 1, a, x, (val_t) 0, y );
  }
  */

}}}}

#endif // BOOST_BINDINGS_BLAS_BLAS2_HPP
