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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_VECTOR_TRAITS_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_VECTOR_TRAITS_HPP

#include <boost/numeric/bindings/traits/config.hpp> 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

#include <boost/numeric/bindings/traits/detail/generate_const.hpp> 
#include <boost/type_traits/remove_const.hpp> 
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
#  include <boost/type_traits/is_same.hpp> 
#  include <boost/static_assert.hpp> 
#endif 

namespace boost { namespace numeric { namespace bindings { namespace traits {

  /// default_vector_traits is just a base-class that can be
  /// used as the default vector_traits and the different
  /// specialisation to automatically define most of the
  /// functions.
  template <typename V, typename T = typename V::value_type >
  struct default_vector_traits {
    typedef T                                                    value_type; 
    typedef typename detail::generate_const<V,value_type>::type* pointer;      // if V is const, pointer will be a const value_type*

    static pointer storage (V& v) { return &v[0]; }
    static int size (V& v) { return static_cast<int>(v.size()); } 
    static int stride (V&) { return 1; } 
  }; 

  // vector_detail_traits is used to implement specializations of vector_traits.
  // VIdentifier is the vector_type without const, while VType can have a const.
  // VIdentifier is used to write template specializations for VType and const VType.
  // e.g.  vector_detail_traits< std::vector<int>, std::vector<int> const >
  // e.g.  vector_detail_traits< std::vector<int>, std::vector<int> >
  // Note that  boost::remove_const<VType>::type == VIdentifier.
  template <typename VIdentifier, typename VType>
  struct vector_detail_traits : default_vector_traits<VType, typename VType::value_type > 
  {
    typedef VIdentifier identifier_type; 
    typedef VType       vector_type; 
  };

  // vector_traits<> generic version: 
  template <typename V>
  struct vector_traits : vector_detail_traits< typename boost::remove_const<V>::type, V > {}; 


  ///////////////////////////
  //
  // free accessor functions: 
  //
  ///////////////////////////

  template <typename V>
  inline 
  typename vector_traits<V>::pointer vector_storage (V& v) { 
    return vector_traits<V>::storage (v); 
  }

  template <typename V>
  inline
  int vector_size (V& v) { 
    return vector_traits<V>::size (v); 
  }

  template <typename V>
  inline
  int vector_stride (V& v) { 
    return vector_traits<V>::stride (v); 
  }

}}}}

#else // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

#include <boost/numeric/bindings/traits/vector_raw.hpp> 

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_VECTOR_TRAITS_HPP
