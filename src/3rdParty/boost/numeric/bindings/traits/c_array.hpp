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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_C_ARRAY_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_C_ARRAY_H

#include <boost/numeric/bindings/traits/config.hpp> 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#include <boost/numeric/bindings/traits/vector_traits.hpp>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  // built-in array
  template <typename T, std::size_t N, typename V> 
  struct vector_detail_traits<T[N], V> {

#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same<T[N], typename boost::remove_const<V>::type>::value) 
    );
#endif

    typedef T identifier_type [N]; // C++ syntax is amazing ;o) 
    typedef V vector_type;
    typedef T value_type; 
    typedef typename detail::generate_const<V, T>::type* pointer; 

    static pointer storage (vector_type& v) { return v; }
    static int size (vector_type&) { return N; } 
    static int stride (vector_type&) { return 1; } 
  }; 

}}}}

#else // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#error with your compiler plain C array cannot be used in bindings 

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_C_ARRAY_H
