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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_STD_VECTOR_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_STD_VECTOR_H

#include <boost/numeric/bindings/traits/vector_traits.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#include <vector>


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // std::vector<>
  template <typename T, typename Alloc, typename V>
  struct vector_detail_traits<std::vector<T, Alloc>, V> 
  : default_vector_traits< V, T >
  {

#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< std::vector<T, Alloc>, typename boost::remove_const<V>::type >::value) );
#endif

    typedef std::vector<T,Alloc>                            identifier_type;  
    typedef V                                               vector_type;
    typedef typename default_vector_traits< V, T >::pointer pointer;

    static pointer storage (vector_type& v) { return &v.front(); }
  }; 

}}}}  

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_STD_VECTOR_H
