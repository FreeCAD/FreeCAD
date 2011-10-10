/*
 * 
 * Copyright (c) 2002, 2003 Kresimir Fresl, Toon Knapen and Karl Meerbergen
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_HERMITIAN_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_HERMITIAN_H

#include <boost/numeric/bindings/traits/traits.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/hermitian.hpp> 
#endif 
#include <boost/numeric/bindings/traits/ublas_matrix.hpp>
#include <boost/numeric/bindings/traits/detail/ublas_uplo.hpp>


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // ublas::hermitian_matrix<>
  template <typename T, typename F1, typename F2, typename A, typename M>
  struct matrix_detail_traits<boost::numeric::ublas::hermitian_matrix<T, F1, F2, A>, M>
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same<boost::numeric::ublas::hermitian_matrix<T, F1, F2, A>, typename boost::remove_const<M>::type>::value) );
#endif
#ifdef BOOST_BINDINGS_FORTRAN
    BOOST_STATIC_ASSERT((boost::is_same<
      typename F2::orientation_category, 
      boost::numeric::ublas::column_major_tag
    >::value)); 
#endif 

    typedef boost::numeric::ublas::hermitian_matrix<T, F1, F2, A> identifier_type;
    typedef M                                                     matrix_type;

    typedef hermitian_packed_t matrix_structure; 
    typedef typename detail::ublas_ordering<
      typename F2::orientation_category
    >::type ordering_type; 
    typedef typename detail::ublas_uplo< F1 >::type uplo_type; 

    typedef T                                           value_type ; 
    typedef typename detail::generate_const<M,T>::type* pointer ; 

    static pointer storage (matrix_type& hm) {
      typedef typename detail::generate_const<M,A>::type array_type ;
      return vector_traits<array_type>::storage (hm.data()); 
    }
    static int size1 (matrix_type& hm) { return hm.size1(); } 
    static int size2 (matrix_type& hm) { return hm.size2(); }
    static int storage_size (matrix_type& hm) { 
      return (size1 (hm) + 1) * size2 (hm) / 2; 
    }
  }; 


  namespace detail {
     template <typename M>
     int matrix_bandwidth( M const& m, upper_t ) {
        return matrix_traits<M const>::upper_bandwidth( m ) ;
     }

     template <typename M>
     int matrix_bandwidth( M const& m, lower_t ) {
        // When the lower triangular band matrix is stored the
	// upper bandwidth must be zero
	assert( 0 == matrix_traits<M const>::upper_bandwidth( m ) ) ;
        return matrix_traits<M const>::lower_bandwidth( m ) ;
     }
  } // namespace detail

  // ublas::hermitian_adaptor<>
  template <typename M, typename F1, typename MA>
  struct matrix_detail_traits<boost::numeric::ublas::hermitian_adaptor<M, F1>, MA>
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same<boost::numeric::ublas::hermitian_adaptor<M, F1>, typename boost::remove_const<MA>::type>::value) );
#endif

    typedef boost::numeric::ublas::hermitian_adaptor<M, F1> identifier_type;
    typedef MA                                              matrix_type; 
    typedef hermitian_t                                     matrix_structure; 
    typedef typename matrix_traits<M>::ordering_type        ordering_type; 
    typedef typename detail::ublas_uplo< F1 >::type         uplo_type; 

    typedef typename M::value_type                                 value_type; 
    typedef typename detail::generate_const<MA, value_type>::type* pointer; 

  private:
    typedef typename detail::generate_const<MA, typename MA::matrix_closure_type>::type m_type; 

  public:
    static pointer storage (matrix_type& hm) {
      return matrix_traits<m_type>::storage (hm.data());
    }
    static int size1 (matrix_type& hm) { return hm.size1(); } 
    static int size2 (matrix_type& hm) { return hm.size2(); }
    static int storage_size (matrix_type& hm) { 
      return size1 (hm) * size2 (hm); 
    }
    static int leading_dimension (matrix_type& hm) {
      return matrix_traits<m_type>::leading_dimension (hm.data()); 
    }
    // For banded M
    static int upper_bandwidth(matrix_type& hm) {
       return detail::matrix_bandwidth( hm.data(), uplo_type() );
    }
    static int lower_bandwidth(matrix_type& hm) {
       return detail::matrix_bandwidth( hm.data(), uplo_type() );
    }
  }; 

}}}}

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_HERMITIAN_H
