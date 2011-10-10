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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_BANDED_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_BANDED_H

#include <boost/numeric/bindings/traits/traits.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/banded.hpp> 
#endif 
#include <boost/numeric/bindings/traits/detail/ublas_ordering.hpp>

#if defined (BOOST_NUMERIC_BINDINGS_FORTRAN) || !defined (BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK)
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/same_traits.hpp>
#endif


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // ublas::matrix_banded<>
  // When orientation_category==row_major_tag then the ublas banded format corresponds to
  // the LAPACK band format.
  // Specialization using matrix_detail_traits so that we can specialize for
  // matrix_detail_traits< banded<T, F, ArrT>, banded<T, F, ArrT> >
  // matrix_detail_traits< banded<T, F, ArrT>, banded<T, F, ArrT> const >
  // at once.
  template <typename T, typename F, typename ArrT, typename M>
  struct matrix_detail_traits< boost::numeric::ublas::banded_matrix<T, F, ArrT>, M > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same<boost::numeric::ublas::banded_matrix<T, F, ArrT>, typename boost::remove_const<M>::type>::value) );
#endif
#ifdef BOOST_NUMERIC_BINDINGS_FORTRAN
    BOOST_STATIC_ASSERT((boost::is_same<
      typename F::orientation_category, 
      boost::numeric::ublas::row_major_tag
    >::value)); 
#endif 

    typedef boost::numeric::ublas::banded_matrix<T, F, ArrT>   identifier_type ;
    typedef M                                                  matrix_type;
    typedef banded_t                                           matrix_structure; 
    typedef typename detail::ublas_ordering<
      typename F::orientation_category
    >::type                                             ordering_type; 

    typedef T                                           value_type; 
    typedef typename detail::generate_const<M,T>::type* pointer; 

    static pointer storage (matrix_type& m) {
      typedef typename detail::generate_const<M,ArrT>::type array_type ;
      return vector_traits<array_type>::storage (m.data()); 
    }
    static int size1 (matrix_type& m) { return m.size1(); } 
    static int size2 (matrix_type& m) { return m.size2(); }
    static int lower_bandwidth (matrix_type& m) { return m.lower() ; }
    static int upper_bandwidth (matrix_type& m) { return m.upper() ; }
    static int storage_size (matrix_type& m) { return size1 (m) * size2 (m); }
    static int leading_dimension (matrix_type& m) {
      typedef typename identifier_type::orientation_category                      orientation_category; 
      return detail::ublas_banded_ordering<orientation_category>::leading_dimension(m) ;
    }

    // stride1 == distance (m (i, j), m (i+1, j)) 
    static int stride1 (matrix_type& m) { 
      typedef typename identifier_type::orientation_category                      orientation_category; 
      return detail::ublas_banded_ordering<orientation_category>::stride1(m) ;
    } 
    // stride2 == distance (m (i, j), m (i, j+1)) 
    static int stride2 (matrix_type& m) { 
      typedef typename identifier_type::orientation_category                      orientation_category; 
      return detail::ublas_banded_ordering<orientation_category>::stride2(m) ;
    }
  }; 


}}}}

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_BANDED_H
