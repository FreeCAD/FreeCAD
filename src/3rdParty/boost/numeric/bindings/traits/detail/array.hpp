/*
 * 
 * Copyright (c) Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_ARRAY_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_ARRAY_HPP

#include <boost/numeric/bindings/traits/vector_traits.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#include <boost/numeric/bindings/traits/detail/array_impl.hpp>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  template <typename T>
  struct vector_traits<detail::array<T> > {
    typedef T value_type;
    typedef T* pointer; 

    static pointer storage (detail::array<T>& a) { return a.storage(); }
    static int size (detail::array<T>& a) { return a.size(); } 
  }; 
  

}}}}

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_ARRAY_HPP
