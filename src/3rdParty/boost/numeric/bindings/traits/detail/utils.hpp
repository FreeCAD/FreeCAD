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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_UTILS_HPP
#define BOOST_NUMERIC_BINDINGS_TRAITS_DETAIL_UTILS_HPP

// #include <cstring> 
#include <iterator>
#include <boost/numeric/bindings/traits/type_traits.hpp>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  namespace detail {

    // complex array => real & imaginary arrays
    template <typename CIt, typename RIt> 
    inline 
    void disentangle (CIt c, CIt c_end, RIt rr, RIt ri) {
      for (; c != c_end; ++c, ++rr, ++ri) {
        *rr = traits::real (*c); 
        *ri = traits::imag (*c); 
      }
    }
    // real & imaginary arrays => complex array
    template <typename RIt, typename CIt> 
    inline 
    void interlace (RIt r, RIt r_end, RIt ri, CIt c) {
      typedef typename std::iterator_traits<CIt>::value_type cmplx_t;
#ifdef BOOST_NUMERIC_BINDINGS_BY_THE_BOOK
      for (; r != r_end; ++r, ++ri, ++c) 
        *c = cmplx_t (*r, *ri); 
#else
      typedef typename type_traits<cmplx_t>::real_type real_t; 
      real_t *cp = reinterpret_cast<real_t*> (&*c);
      for (; r != r_end; ++r, ++ri) {
        *cp = *r; ++cp;
        *cp = *ri; ++cp;
      }
#endif 
    }    


    // converts real/complex to int
    inline int to_int (float f) { return static_cast<int> (f); }
    inline int to_int (double d) { return static_cast<int> (d); }
    inline int to_int (traits::complex_f const& cf) { 
      return static_cast<int> (traits::real (cf)); 
    }
    inline int to_int (traits::complex_d const& cd) { 
      return static_cast<int> (traits::real (cd)); 
    }

  }

}}}}

#endif 
