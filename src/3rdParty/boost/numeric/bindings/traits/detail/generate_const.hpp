/*
 * 
 * Copyright (c) 2002, 2003 Kresimir Fresl, Toon Knapen and Karl Meerbergen
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * First author acknowledges the support of the Faculty of Civil 
 * Engineering, University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_GENERATE_CONST_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_GENERATE_CONST_HPP

#include <boost/numeric/bindings/traits/config.hpp> 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

namespace boost { namespace numeric { namespace bindings { namespace traits { namespace detail {
 
  /// Copy const from V to X if present

  template <typename V, typename X>
  struct generate_const {
     typedef X type; 
  };

  template <typename V, typename X>
  struct generate_const< const V, X > {
     typedef X const type; 
  };

  template <typename V, typename X>
  struct generate_const< V const, X const > {
     typedef X const type; 
  };

  template <typename T, int N, typename X>
  struct generate_const< const T[N], X > {
     typedef X const type; 
  };

  template <typename T, int N, typename X>
  struct generate_const< const T[N], X const > {
     typedef X const type; 
  };

}}}}}

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_GENERATE_CONST_HPP
