/*
 * 
 * Copyright (c) Kresimir Fresl 2002 
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS_ENUM_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS_ENUM_HPP

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/atlas/cblas_inc.hpp>
#ifdef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
#  include <boost/numeric/ublas/config.hpp> 
#endif 

namespace boost { namespace numeric { namespace bindings { 

  namespace atlas {

    template <typename Ord> struct storage_order {};
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    template<> struct storage_order<traits::row_major_t> {
      // 'ename' - como (in enum_cast<>): 
      //    a template argument may not reference an unnamed type
      enum ename { value = CblasRowMajor };
    };
    template<> struct storage_order<traits::column_major_t> {
      enum ename { value = CblasColMajor };
    };
#else
    template<> struct storage_order<boost::numeric::ublas::row_major_tag> {
      enum ename { value = CblasRowMajor };
    };
    template<> struct storage_order<boost::numeric::ublas::column_major_tag> {
      enum ename { value = CblasColMajor };
    };
#endif 

    template <typename UpLo> struct uplo_triang {};
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    template<> struct uplo_triang<traits::upper_t> {
      enum ename { value = CblasUpper };
    };
    template<> struct uplo_triang<traits::lower_t> {
      enum ename { value = CblasLower };
    };
#else
    template<> struct uplo_triang<boost::numeric::ublas::upper_tag> {
      enum ename { value = CblasUpper };
    };
    template<> struct uplo_triang<boost::numeric::ublas::lower_tag> {
      enum ename { value = CblasLower };
    };
#endif 

    // g++ 3.0.4 rejects 'static_cast<E1> (e2)'
    template <typename E1, typename E2>
    E1 enum_cast (E2 e2) {
      return static_cast<E1> (static_cast<int> (e2)); 
    }

  }

}}}

#endif 
