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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_TNT_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_TNT_H

// Roldan Pozo's TNT (Template Numerical Toolkit)
// see: http://math.nist.gov/tnt/index.html

#include <boost/numeric/bindings/traits/config.hpp> 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#include <boost/numeric/bindings/traits/traits.hpp>
#include <tnt/tnt_array1d.h>
#include <tnt/tnt_fortran_array1d.h>
#include <tnt/tnt_array2d.h>
#include <tnt/tnt_fortran_array2d.h>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  // TNT::Array1D<>
  template <typename T, typename V>
  struct vector_detail_traits<TNT::Array1D<T>, V> 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same< 
         TNT::Array1D<T>, typename boost::remove_const<V>::type 
       >::value) );
#endif

    typedef TNT::Array1D<T> identifier_type; 
    typedef V vector_type;
    typedef T value_type; 
    typedef typename detail::generate_const<V,T>::type* pointer; 

    static pointer storage (vector_type& v) { return &v[0]; }
    static int size (vector_type& v) { return v.dim(); } 
    static int stride (vector_type& v) { return 1; } 
  }; 

  // TNT::Fortran_Array1D<>
  template <typename T, typename V>
  struct vector_detail_traits<TNT::Fortran_Array1D<T>, V> 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same< 
         TNT::Fortran_Array1D<T>, typename boost::remove_const<V>::type 
       >::value) );
#endif

    typedef TNT::Fortran_Array1D<T> identifier_type; 
    typedef V vector_type;
    typedef T value_type; 
    typedef typename detail::generate_const<V,T>::type* pointer; 

    static pointer storage (vector_type& v) { return &v(1); }
    static int size (vector_type& v) { return v.dim(); } 
    static int stride (vector_type& v) { return 1; } 
  }; 


  // TNT::Array2D<>
  template <typename T, typename M>
  struct matrix_detail_traits<TNT::Array2D<T>, M> 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same< 
         TNT::Array2D<T>, typename boost::remove_const<M>::type 
       >::value) );
#endif

    typedef TNT::Array2D<T> identifier_type; 
    typedef M matrix_type;
    typedef general_t matrix_structure; 
    typedef row_major_t ordering_type; 

    typedef T value_type; 
    typedef typename detail::generate_const<M,T>::type* pointer; 

    static pointer storage (matrix_type& m) { return m[0]; }
    static int size1 (matrix_type& m) { return m.dim1(); } 
    static int size2 (matrix_type& m) { return m.dim2(); } 
    static int storage_size (matrix_type& m) { 
      return size1 (m) * size2 (m); 
    }
    static int leading_dimension (matrix_type& m) { return m.dim2(); } 
  }; 

  // TNT::Fortran_Array2D<>
  template <typename T, typename M>
  struct matrix_detail_traits<TNT::Fortran_Array2D<T>, M> {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same< 
         TNT::Fortran_Array2D<T>, typename boost::remove_const<M>::type 
       >::value) );
#endif

    typedef TNT::Fortran_Array2D<T> identifier_type; 
    typedef M matrix_type;
    typedef general_t matrix_structure; 
    typedef column_major_t ordering_type; 

    typedef T value_type; 
    typedef typename detail::generate_const<M,T>::type* pointer; 

    static pointer storage (matrix_type& m) { return &m(1, 1); }
    static int size1 (matrix_type& m) { return m.dim1(); } 
    static int size2 (matrix_type& m) { return m.dim2(); } 
    static int storage_size (matrix_type& m) { 
      return size1 (m) * size2 (m); 
    }
    static int leading_dimension (matrix_type& m) { return m.dim1(); } 
  }; 

}}}}  

#else // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#error with your compiler TNT cannot be used in bindings

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_TNT_H
