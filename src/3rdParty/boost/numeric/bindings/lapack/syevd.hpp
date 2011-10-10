// 
// Copyright (c) Thomas Klimpel 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_SYEVD_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_SYEVD_HPP

#include <boost/numeric/bindings/lapack/heevd.hpp>

namespace boost { namespace numeric { namespace bindings { 
  namespace lapack {

    template <typename A, typename W, typename Work>
    inline int syevd (
      char jobz, char uplo, A& a,
      W& w, Work work = optimal_workspace() ) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      typedef typename A::value_type                               value_type ;
      typedef typename traits::type_traits< value_type >::real_type real_type ;
      BOOST_STATIC_ASSERT((boost::is_same<value_type, real_type>::value));
#endif

      return heevd (jobz, uplo, a, w, work);
    }
  }
}}}

#endif
