/*
 * 
 * Copyright (c) Kresimir Fresl and Toon Knapen 2002, 2003 
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * First author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_TYPE_TRAITS_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_TYPE_TRAITS_HPP

#include <boost/numeric/bindings/traits/config.hpp>
#include <boost/numeric/bindings/traits/type.h>
#include <boost/numeric/bindings/traits/type.hpp>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  template <typename Real> 
  struct type_traits { 
#ifdef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    typedef Real type; 
    typedef Real real_type; 
#endif 
  };
  template<>
  struct type_traits<float> { 
    typedef float type; 
    typedef float real_type; 
  };
  template<> 
  struct type_traits<double> { 
    typedef double type; 
    typedef double real_type; 
  };
  template<> 
  struct type_traits<complex_f> { 
    typedef complex_f type; 
    typedef float real_type; 
  };
  template<> 
  struct type_traits<complex_d> { 
    typedef complex_d type; 
    typedef double real_type; 
  };


  inline const fcomplex_t*  complex_ptr(const complex_f* c) { return reinterpret_cast<const fcomplex_t*>( c ) ; }
  inline const dcomplex_t*  complex_ptr(const complex_d* c) { return reinterpret_cast<const dcomplex_t*>( c ) ; }

  inline       fcomplex_t*  complex_ptr(      complex_f* c) { return reinterpret_cast<      fcomplex_t*>( c ) ; }
  inline       dcomplex_t*  complex_ptr(      complex_d* c) { return reinterpret_cast<      dcomplex_t*>( c ) ; }

}}}}

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_TYPE_TRAITS_HPP

