/*
 * 
 * Copyright (c) 2002, 2003 Kresimir Fresl, Toon Knapen and Karl Meerbergen
 *
 * Permission to copy, modify, use and distribute this software 
 * for any non-commercial or commercial purpose is granted provided 
 * that this license appear on all copies of the software source code.
 *
 * Authors assume no responsibility whatsoever for its use and makes 
 * no guarantees about its quality, correctness or reliability.
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_STD_VALARRAY_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_STD_VALARRAY_H

#include <boost/numeric/bindings/traits/config.hpp> 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#include <boost/numeric/bindings/traits/vector_traits.hpp>
#include <valarray>


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // std::valarray<>
  template <typename T, typename V> 
  struct vector_detail_traits<std::valarray<T>, V> 
  : default_vector_traits<V, T> 
  {

#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same< 
         std::valarray<T>, typename boost::remove_const<V>::type 
       >::value) 
    );
#endif

    typedef std::valarray<T> identifier_type; 
    typedef V vector_type;
    typedef typename default_vector_traits<V, T>::pointer pointer;

    static pointer storage (vector_type& va) { 
      std::valarray<T>& ncva = const_cast<std::valarray<T>&> (va);
      return &ncva[0];
    }
  }; 

}}}}  

#else // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#error with your compiler std::valarray<> cannot be used in bindings

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_STD_VALARRAY_H
