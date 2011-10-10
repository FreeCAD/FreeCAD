// 
// Copyright (c) Thomas Klimpel 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_SYEVX_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_SYEVX_HPP

#include <boost/numeric/bindings/lapack/heevx.hpp>

namespace boost { namespace numeric { namespace bindings { 
  namespace lapack {
    template <typename A, typename T, typename W, typename Z, typename IFail, typename Work>
    int syevx (
      char jobz, char range, A& a, T vl, T vu, int il, int iu, T abstol, int& m,
      W& w, Z& z, IFail& ifail, Work work = optimal_workspace() ) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      typedef typename A::value_type                               value_type ;
      typedef typename traits::type_traits< value_type >::real_type real_type ;
      BOOST_STATIC_ASSERT((boost::is_same<value_type, real_type>::value));
#endif

      return heevx (jobz, range, a, vl, vu, il, iu, abstol, m, w, z, ifail, work);
    }
  }
}}}

#endif
